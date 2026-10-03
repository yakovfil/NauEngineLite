// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/audio/audio_container.hpp"

#include <chrono>
#include <numeric>
#include <random>
#include <string>
#include <vector>

NAU_AUDIO_BEGIN

// ** AudioAssetContainer::Impl

class AudioAssetContainer::Impl
{
public:
    Impl(const std::string& name);

    void add(AudioAssetPtr asset);

public:
    AudioContainerKind    kind;
    const std::string name;
    AudioAssetList        assets;
};

AudioAssetContainer::Impl::Impl(const std::string& name) :
    kind(AudioContainerKind::Sequence),
    name(name)
{

}

void AudioAssetContainer::Impl::add(AudioAssetPtr asset)
{

}


// ** AudioAssetContainer

AudioAssetContainer::AudioAssetContainer(const std::string& name) :
    m_pimpl(std::make_unique<Impl>(name))
{

}

AudioAssetContainer::~AudioAssetContainer() = default;

AudioSourcePtr AudioAssetContainer::instantiate()
{
    static std::random_device randomDevice;
    static std::mt19937 randomEngine(randomDevice());

    if (m_pimpl->assets.empty()) return nullptr;

    auto result = std::make_shared<AudioContainer>();
    if (const auto kind = m_pimpl->kind; kind == AudioContainerKind::Sequence) {
        for (auto asset : m_pimpl->assets) {
            result->addSource(asset->instantiate());
        }
    } else if (kind == AudioContainerKind::Random) {
        std::uniform_int_distribution<int> uniform_dist(0, m_pimpl->assets.size() - 1);
        AudioAssetPtr randomAsset = m_pimpl->assets[uniform_dist(randomEngine)];
        result->addSource(randomAsset->instantiate());
    } else if (kind == AudioContainerKind::Shuffle) {
        std::vector<int> indices(m_pimpl->assets.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), randomEngine);
        for (auto index : indices) {
            result->addSource(m_pimpl->assets[index]->instantiate());
        }
    // TODO:
    // } else if (kind == AudioContainerKind::Mix) {
    // 
    }
    return result;
}

std::string AudioAssetContainer::name() const
{
    return m_pimpl->name;
}

std::vector<AudioAssetPtr>::iterator AudioAssetContainer::begin()
{
    return m_pimpl->assets.begin();
}

std::vector<AudioAssetPtr>::iterator AudioAssetContainer::end()
{
    return m_pimpl->assets.end();
}

AudioContainerKind AudioAssetContainer::kind() const
{
    return m_pimpl->kind;
}

void AudioAssetContainer::setKind(AudioContainerKind kind)
{
    m_pimpl->kind = kind;
    notifyAll();
}

void AudioAssetContainer::add(AudioAssetPtr asset)
{
    m_pimpl->assets.push_back(asset);
    notifyAll();
}

void AudioAssetContainer::remove(AudioAssetPtr asset)
{
    if (auto element = std::find(m_pimpl->assets.begin(), m_pimpl->assets.end(), asset); element != m_pimpl->assets.end()) {
        m_pimpl->assets.erase(element);
        notifyAll();
    } else {
        NAU_LOG_WARNING("Trying to remove an asset that doesn't belong to this container!");
    }
}

bool AudioAssetContainer::empty() const
{
    return m_pimpl->assets.empty();
}


// ** AudioContainer

void AudioContainer::play()
{
    if (!m_current) return;
    m_current->play();
}

void AudioContainer::stop()
{
    if (!m_current) return;
    m_current->stop();
}

void AudioContainer::pause()
{
    if (!m_current) return;
    m_current->pause();
}
  
void AudioContainer::seek(std::chrono::milliseconds ms)
{
    if (!m_current) return;
    auto totalDuration = std::chrono::milliseconds(0);
    for (auto source : m_sources) {
        const auto sourceDuration = source->duration();
        if (const auto msCurrent = ms - totalDuration; msCurrent <= sourceDuration) {
            m_current = source;
            m_current->seek(msCurrent);
            return;
        }
        totalDuration += sourceDuration;
    }
    NAU_LOG_ERROR("Incorrect seek position within audio source!");
}

std::chrono::milliseconds AudioContainer::duration() const
{
    return std::accumulate(m_sources.begin(), m_sources.end(), std::chrono::milliseconds(0), 
        [](std::chrono::milliseconds ms, AudioSourcePtr source) {
            return ms + source->duration();
        });
}

std::chrono::milliseconds AudioContainer::position() const
{
    return m_current->position();
}

bool AudioContainer::isAtEnd() const
{
    if (!m_current) return false;
    return m_current->isAtEnd();
}

bool AudioContainer::isPlaying() const
{
    if (!m_current) return false;
    return m_current->isPlaying();
}

void AudioContainer::setEndCallback(SoundCompletionCallback callback)
{
    if (m_sources.empty()) return;
    m_sources.back()->setEndCallback(callback);
}

void AudioContainer::addSource(AudioSourcePtr source)
{
    if (m_sources.empty()) {
        m_current = source;
    } else {
        m_sources.back()->playNext(source);
    }

    m_sources.push_back(source);
}

NAU_AUDIO_END
