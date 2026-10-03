// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/audio/audio_engine.hpp"

#include <string>

#include "audio_backend_miniaudio.hpp"

NAU_AUDIO_BEGIN

// ** IAudioEngine

AudioAssetContainerList IAudioEngine::containerAssets()
{
    return m_containers;
}

std::unique_ptr<IAudioEngine> IAudioEngine::create(Backend backend)
{
    if (backend == Backend::Miniaudio) {
        return std::make_unique<AudioEngineMiniaudio>();
    } else {
        NAU_ASSERT(false, "Unknown audio API");
        return nullptr;
    }
}

AudioAssetList IAudioEngine::assets()
{
    AudioAssetList result;
    for (auto asset : audioAssets()) result.push_back(asset);
    for (auto asset : containerAssets()) result.push_back(asset);
    return result;
}

AudioAssetContainerPtr IAudioEngine::createContainer(const std::string& name)
{
    auto container = std::make_shared<AudioAssetContainer>(name);
    m_containers.emplace_back(container);
    return container;
}

NAU_AUDIO_END
