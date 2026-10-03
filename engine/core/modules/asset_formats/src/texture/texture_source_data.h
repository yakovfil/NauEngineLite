// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
//

#pragma once

#include <span>

#include "nau/assets/texture_asset_accessor.h"
#include "nau/io/file_system.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/result.h"
#include "tinyimageformat_base.h"

namespace nau
{
    /**
     */
    class TextureSourceData
    {
    public:
        static Result<TextureSourceData> loadFromStream(io::IStreamReader::Ptr stream, RuntimeReadonlyDictionary::Ptr importSettings, TinyImageFormat forceFormat = TinyImageFormat_UNDEFINED);

        TextureSourceData() = default;
        TextureSourceData(const TextureSourceData&) = delete;
        TextureSourceData(TextureSourceData&&);
        ~TextureSourceData();

        TextureSourceData& operator=(const TextureSourceData&) = delete;
        TextureSourceData& operator=(TextureSourceData&&);

        explicit operator bool() const;

        bool isCompressed() const;
        unsigned getWidth() const;
        unsigned getHeight() const;
        unsigned getNumMipmaps() const;
        TinyImageFormat getFormat() const;

        void copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, std::span<DestTextureData> destination) const;
        const void* getTextureData() const;

    private:
        TextureSourceData(unsigned w, unsigned h, unsigned numMipmaps, TinyImageFormat format, TinyImageFormat compressedFormat, void* data);

        unsigned m_width = 0;
        unsigned m_height = 0;
        unsigned m_numMipmaps = 0;
        TinyImageFormat m_format = TinyImageFormat_UNDEFINED;
        TinyImageFormat m_compressedFormat = TinyImageFormat_UNDEFINED;
        void* m_data = nullptr;
    };
}  // namespace nau
