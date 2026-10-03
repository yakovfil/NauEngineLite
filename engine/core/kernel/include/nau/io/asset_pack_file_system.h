// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <chrono>
#include <string_view>

#include "nau/io/file_system.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/ptr.h"

/**
 * @brief Provides the settings for asset pack file systems and a function to create an asset pack file system.
 */

namespace nau::io
{
    /**
     * @struct AssetPackFileSystemSettings
     * @brief Settings for configuring an asset pack file system.
     * @details This structure defines parameters for caching and other settings specific to an asset pack file system.
     */
    struct AssetPackFileSystemSettings
    {
        size_t maxCacheSize = 5000000;                                   ///< Maximum cache size in bytes.
        std::chrono::seconds lifetimeOfCache = std::chrono::minutes(1);  ///< Lifetime of cache entries.
    };

    /**
     * @brief Creates an asset pack file system.
     * @param assetPackPath Path to the asset pack.
     * @param settings Configuration settings for the asset pack file system.
     * @return Pointer to the created asset pack file system.
     * @details This function initializes a file system for managing asset packs with the specified settings.
     */
    NAU_KERNEL_EXPORT
    IFileSystem::Ptr createAssetPackFileSystem(std::u8string_view assetPackPath, AssetPackFileSystemSettings settings = {});
}  // namespace nau::io
