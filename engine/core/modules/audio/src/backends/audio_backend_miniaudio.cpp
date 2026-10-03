// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "audio_backend_miniaudio.hpp"

#include <string>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define __WIN__ 1
#ifdef __WIN__
#undef PlaySound
#endif


NAU_AUDIO_BEGIN

// ** SoundMiniaudio

class SoundMiniaudio : public IAudioSource
{
    friend class SoundAssetMiniaudio;

public:
    SoundMiniaudio();
    ~SoundMiniaudio();

    // From IAudioSource
    void play() override;
    void stop() override;
    void pause() override;
    void seek(std::chrono::milliseconds ms) override;

    std::chrono::milliseconds duration() const override;
    std::chrono::milliseconds position() const override;
    bool isAtEnd() const override;
    bool isPlaying() const override;


    void setEndCallback(SoundCompletionCallback callback) override;

private:
    std::chrono::milliseconds framesToMilliseconds(ma_uint64 frames) const;
    ma_uint64 millisecondsToFrames(std::chrono::milliseconds ms) const;

private:
    ma_sound                               m_sound;
    std::optional<SoundCompletionCallback> m_endCallback;
};


SoundMiniaudio::SoundMiniaudio()
    : m_sound()
{
}

SoundMiniaudio::~SoundMiniaudio()
{
    ma_sound_uninit(&m_sound);
}

void SoundMiniaudio::play()
{
    ma_result result;
    if (m_endCallback) {
        result = ma_sound_set_end_callback(&m_sound, [](void* data, [[maybe_unused]] ma_sound* sound) {
            const auto& callback = *static_cast<SoundCompletionCallback*>(data);
            callback();
        }, &m_endCallback);
    } else {
        result = ma_sound_set_end_callback(&m_sound, nullptr, nullptr);
    }

    if (result != MA_SUCCESS) {
        NAU_LOG_ERROR("Failed to set audio callback");
        return;
    }

    result = ma_sound_start(&m_sound);
    if (result != MA_SUCCESS) {
        NAU_LOG_ERROR("Failed to play audio");
        return;
    }
}

void SoundMiniaudio::stop()
{
    if (const ma_result result = ma_sound_stop(&m_sound); result != MA_SUCCESS) {
        NAU_LOG_ERROR("Failed to stop audio source. MA error: {}", static_cast<int>(result));
    }
}

void SoundMiniaudio::pause()
{
    stop();
}

void SoundMiniaudio::seek(std::chrono::milliseconds ms)
{
    const auto frame = millisecondsToFrames(ms);
    if (const ma_result result = ma_sound_seek_to_pcm_frame(&m_sound, frame); result != MA_SUCCESS) {
        NAU_LOG_ERROR("Failed to seek audio source to frame {}. MA error: ", frame, static_cast<int>(result));
    }
}

std::chrono::milliseconds SoundMiniaudio::duration() const
{
    ma_uint64 frames;
    const ma_result result = ma_sound_get_length_in_pcm_frames(&const_cast<ma_sound&>(m_sound), &frames);
    if (result != MA_SUCCESS) {
        NAU_LOG_ERROR("Failed to calculate audio source duration");
        return std::chrono::milliseconds(0);
    }
    
    return framesToMilliseconds(frames);
}

std::chrono::milliseconds SoundMiniaudio::position() const
{
    const ma_uint64 frames = ma_sound_get_time_in_pcm_frames(&m_sound);
    return framesToMilliseconds(frames);
}

bool SoundMiniaudio::isAtEnd() const
{
    return ma_sound_at_end(&m_sound);
}

bool SoundMiniaudio::isPlaying() const
{
    return ma_sound_is_playing(&m_sound);
}

void SoundMiniaudio::setEndCallback(SoundCompletionCallback callback)
{
    m_endCallback = !callback ? std::nullopt : std::optional(callback);
}

std::chrono::milliseconds SoundMiniaudio::framesToMilliseconds(ma_uint64 frames) const
{
    return std::chrono::milliseconds(frames * 1000 / ma_engine_get_sample_rate(ma_sound_get_engine(&m_sound)));
}

ma_uint64 SoundMiniaudio::millisecondsToFrames(std::chrono::milliseconds ms) const
{
    return (ms.count() * ma_engine_get_sample_rate(ma_sound_get_engine(&m_sound))) / 1000;
}


// ** SoundAssetMiniaudio

class SoundAssetMiniaudio : public IAudioAsset
{
    friend class AudioEngineMiniaudio;

public:
    SoundAssetMiniaudio(const std::string& name, ma_engine& engine);

    AudioSourcePtr instantiate() override;
    std::string name() const override;

private:
    const std::string m_name;
    ma_engine&           m_engine;
    ma_sound             m_sound;
};

SoundAssetMiniaudio::SoundAssetMiniaudio(const std::string& name, ma_engine& engine) :
    m_name(name),
    m_engine(engine),
    m_sound()
{
}

AudioSourcePtr SoundAssetMiniaudio::instantiate()
{
    auto sound = std::make_shared<SoundMiniaudio>();
    ma_sound_init_copy(&m_engine, &m_sound, 0, nullptr, &sound->m_sound);
    return sound;
}

std::string SoundAssetMiniaudio::name() const
{
    return m_name;
}


// ** AudioEngineMiniaudio::Impl

class AudioEngineMiniaudio::Impl
{
public:
    Impl() = default;
    
    void initialize();
    void deinitialize();

    AudioAssetPtr loadSound(const std::string& path, bool stream);

    inline AudioAssetList audioAssets() { return assets; }

public:
    ma_engine       engine;
    AudioAssetList  assets;
};

void AudioEngineMiniaudio::Impl::initialize()
{
    const ma_result result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS) {
        NAU_LOG_CRITICAL("Failed to initialize audio engine! MA error: {}", static_cast<int>(result));
        return;
    }

    NAU_LOG_DEBUG("Audio engine successfully initialized");
}

void AudioEngineMiniaudio::Impl::deinitialize()
{
    ma_engine_uninit(&engine);
    NAU_LOG_DEBUG("Audio engine successfully deinitialized");
}

AudioAssetPtr AudioEngineMiniaudio::Impl::loadSound(const std::string& path, bool stream)
{
    auto asset = std::make_shared<SoundAssetMiniaudio>(path, engine);
    const int flags = stream ? MA_SOUND_FLAG_STREAM : 0;
    const ma_result result = ma_sound_init_from_file(&engine, path.c_str(), flags, NULL, NULL, &asset->m_sound);
    if (result != MA_SUCCESS) {
        return nullptr;
    }

    NAU_LOG_INFO("Sound at {} loaded successfully", path);

    assets.emplace_back(asset);
    return asset;
}


// ** AudioEngineMiniaudio

AudioEngineMiniaudio::AudioEngineMiniaudio()
    : m_pimpl(std::make_unique<Impl>())
{
}

AudioEngineMiniaudio::~AudioEngineMiniaudio() = default;

void AudioEngineMiniaudio::initialize()
{
    m_pimpl->initialize();
}

void AudioEngineMiniaudio::deinitialize()
{
    m_pimpl->deinitialize();
}

void AudioEngineMiniaudio::update()
{
}

AudioAssetPtr AudioEngineMiniaudio::loadSound(const std::string& path)
{
    return m_pimpl->loadSound(path, false);
}

AudioAssetPtr AudioEngineMiniaudio::loadStream(const std::string& path)
{
    return m_pimpl->loadSound(path, true);
}

AudioAssetList AudioEngineMiniaudio::audioAssets()
{
    return m_pimpl->audioAssets();
}

NAU_AUDIO_END
