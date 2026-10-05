// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <chrono>
#include <string_view>

#include "nau/io/asset_pack.h"
#include "nau/io/file_system.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/ptr.h"

/**
 * @brief Provides the settings for asset pack file systems and a function to create an asset pack file system.
 */

namespace nau::io
{
    // Limits apply before allocation and before converting archive integers to size_t.
    struct AssetPackReadOptions
    {
        size_t maxHeaderBytes = 4096;
        size_t maxIndexBytes = 8 * 1024 * 1024;
        size_t maxEntries = 65536;
        size_t maxPathBytes = 4096;
    };

    NAU_KERNEL_EXPORT
    Result<std::string> normalizeAssetPackPath(std::string_view path, size_t maxBytes = 4096);

    // Returns validated entries with absolute offsets in the backing stream.
    NAU_KERNEL_EXPORT
    Result<AssetPackIndexData> readAssetPackIndex(IStreamReader::Ptr stream, size_t length, AssetPackReadOptions options = {});

    // Caller transfers exclusive use of the seekable backing stream's cursor.
    // Open files/streams retain backing ownership even after the filesystem is released.
    NAU_KERNEL_EXPORT
    Result<IFileSystem::Ptr> createAssetPackFileSystem(IStreamReader::Ptr stream, size_t length, AssetPackReadOptions options = {});

    // Opens a native/virtual libc file (including an Emscripten preloaded file).
    NAU_KERNEL_EXPORT
    Result<IFileSystem::Ptr> openAssetPackFileSystem(std::string_view path, AssetPackReadOptions options = {});

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
