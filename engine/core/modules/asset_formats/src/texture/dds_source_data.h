// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <span>

#include "nau/assets/texture_asset_accessor.h"
#include "nau/io/stream.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/result.h"
#include "tinyimageformat_base.h"

typedef struct TinyDDS_Context* TinyDDS_ContextHandle;

namespace nau
{
    struct DDSReader
    {
        DDSReader(DDSReader&&) = delete;
        DDSReader(const DDSReader&) = delete;
        DDSReader(io::IStreamReader::Ptr stream);
        ~DDSReader();

        DDSReader& operator=(const DDSReader&) = delete;

        io::IStreamReader::Ptr m_stream;
        TinyDDS_ContextHandle m_data = nullptr;
    };
    /**
     */
    class DDSSourceData
    {
    public:
        static Result<DDSSourceData> loadFromStream(io::IStreamReader::Ptr stream);

        DDSSourceData() = default;
        DDSSourceData(const DDSSourceData&) = delete;
        DDSSourceData(DDSSourceData&&);
        ~DDSSourceData();

        DDSSourceData& operator=(const DDSSourceData&) = delete;
        DDSSourceData& operator=(DDSSourceData&&);

        explicit operator bool() const;

        unsigned getDepth() const;
        TextureType getType() const;
        unsigned getArraySize() const;
        unsigned getWidth() const;
        unsigned getHeight() const;
        unsigned getNumMipmaps() const;
        bool isCompressed() const;
        TinyImageFormat getFormat() const;

        void copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, std::span<DestTextureData> destination);

    private:
        DDSSourceData(io::IStreamReader::Ptr stream);

        std::mutex m_mutex;
        DDSReader* m_reader = nullptr;
    };

}  // namespace nau
