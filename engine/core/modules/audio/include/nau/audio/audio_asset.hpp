// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "audio_common.hpp"
#include "audio_source.hpp"
#include "audio_subscribable.hpp"

NAU_AUDIO_BEGIN

// ** IAudioAsset

using AssetChangeCallback = std::function<void()>;

/**
 * @brief Provides an interface for managing an audio asset.
 */
class IAudioAsset : public Subscribable<AssetChangeCallback>
{
public:

    /**
     * @brief Creates a copy of the audio asset.
     * 
     * @return Copy of this audio asset.
     */
    virtual AudioSourcePtr instantiate() = 0;

    /**
     * @brief Retrieves the audio asset name.
     * 
     * @return  Name of the audio asset.
     */
    virtual std::string name() const = 0;
};

using AudioAssetPtr = std::shared_ptr<IAudioAsset>;
using AudioAssetList = std::vector<AudioAssetPtr>;

NAU_AUDIO_END
