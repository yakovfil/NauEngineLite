// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

// #include <optional>
// #include <string>
// #include <vector>

#include <tinyimageformat.h>

#include <span>

#include "nau/assets/asset_accessor.h"
#include "nau/async/task.h"

namespace nau
{
    enum class TextureType
    {
        UNDEFINED,
        TEXTURE_2D,
        TEXTURE_3D,
        TEXTURE_CUBEMAP,
        TEXTURE_ARRAY,
    };
    /**
     */
    struct TextureDescription
    {
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t depth = 0;
        uint32_t numMipmaps = 0;
        uint32_t arraySize = 0;
        TinyImageFormat format = TinyImageFormat_UNDEFINED;
        TextureType type = TextureType::UNDEFINED;
        bool isCompressed = false;
    };

    /**
     */
    struct DestTextureData
    {
        void* outputBuffer = nullptr;
        size_t rowsCount = 0;
        size_t rowPitch = 0;
        size_t rowBytesSize = 0;
        size_t slicePitch = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE ITextureAssetAccessor : IAssetAccessor
    {
        NAU_INTERFACE(nau::ITextureAssetAccessor, IAssetAccessor)

        virtual TextureDescription getDescription() const = 0;

        virtual void copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, std::span<DestTextureData> destination) = 0;
    };
}  // namespace nau
