// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./dds_source_data.h"

#include <span>

#include "./texture_utils.h"
#include "nau/diag/logging.h"
#include "nau/service/service_provider.h"
#include "texture_asset_container.h"

// Nothings
// TODO: replace malloc/realloc/free with our allocator
#ifndef STBI_MALLOC
    #define STBI_MALLOC(sz) malloc(sz)
#endif
// #define STBI_REALLOC(p, newsz) realloc(p, newsz)
#ifndef STBI_FREE
    #define STBI_FREE(p) free(p)
#endif

#include "tinyimageformat.h"

#define TINYDDS_IMPLEMENTATION
#include "tinydds.h"

namespace nau
{
    namespace
    {
        TinyDDS_Callbacks ddsReadCallbacks
        {
            [](void* user, char const* message)
            {
                NAU_LOG(message);
            },
            [](void* user, size_t size)
            {
                return STBI_MALLOC(size);
            },
            [](void* user, void* memory)
            {
                STBI_FREE(memory);
            },
            [](void* user, void* buffer, size_t byteCount)
            {
                return *static_cast<DDSReader*>(user)->m_stream->read(reinterpret_cast<std::byte*>(buffer), byteCount);
            },
            [](void* user, int64_t offset)
            {
                static_cast<DDSReader*>(user)->m_stream->setPosition(io::OffsetOrigin::Begin, offset);
                return true;
            },
            [](void* user)
            {
                return static_cast<int64_t>(static_cast<DDSReader*>(user)->m_stream->getPosition());
            }
        };
    }  // namespace

    DDSReader::DDSReader(io::IStreamReader::Ptr stream) : m_stream{stream}
    {
        NAU_ASSERT(stream);

        m_data = TinyDDS_CreateContext(&ddsReadCallbacks, this);
        const bool headerOkay = TinyDDS_ReadHeader(m_data);
        if (!headerOkay)
        {
            TinyDDS_DestroyContext(m_data);
            NAU_FAILURE("Could not load dds texture information.");
            return;
        }
        NAU_ASSERT(m_data);
        NAU_ASSERT(TinyImageFormat_FromTinyDDSFormat(TinyDDS_GetFormat(m_data)) != TinyImageFormat_UNDEFINED);
    }

    DDSReader::~DDSReader()
    {
        if (m_data)
        {
            TinyDDS_DestroyContext(m_data);
        }
    }

    Result<DDSSourceData> DDSSourceData::loadFromStream(io::IStreamReader::Ptr stream)
    {
        return DDSSourceData{stream};
    }

    DDSSourceData::DDSSourceData(io::IStreamReader::Ptr stream) :
        m_reader(new DDSReader(stream))
    {
    }
    
    DDSSourceData::~DDSSourceData()
    {
        if(m_reader)
        {
            delete m_reader;
        }
    }

    DDSSourceData::DDSSourceData(DDSSourceData&& other) :
        m_reader(std::exchange(other.m_reader, nullptr))
    {
    }

    DDSSourceData& DDSSourceData::operator=(DDSSourceData&& other)
    {
        m_reader = std::exchange(other.m_reader, nullptr);
        return *this;
    }

    DDSSourceData::operator bool() const
    {
        return m_reader->m_data != nullptr;
    }

    unsigned DDSSourceData::getDepth() const
    {
        return std::max(1U, TinyDDS_Depth(m_reader->m_data));
    }

    TextureType DDSSourceData::getType() const
    {
        TextureType type = TextureType::UNDEFINED;
        if (TinyDDS_IsCubemap(m_reader->m_data))
        {
            type = TextureType::TEXTURE_CUBEMAP;
        }
        else if (TinyDDS_Is2D(m_reader->m_data))
        {
            type = TextureType::TEXTURE_2D;
        }
        else if (TinyDDS_Is3D(m_reader->m_data))
        {
            type = TextureType::TEXTURE_3D;
        }
        else if (TinyDDS_IsArray(m_reader->m_data))
        {
            type = TextureType::TEXTURE_ARRAY;
        }
        return type;
    }

    unsigned DDSSourceData::getArraySize() const
    {
        unsigned arraySize = std::max(1U, TinyDDS_ArraySlices(m_reader->m_data));
        if (TinyDDS_IsCubemap(m_reader->m_data))
        {
            arraySize *= 6;
        }
        return arraySize;
    }

    unsigned DDSSourceData::getWidth() const
    {
        return TinyDDS_Width(m_reader->m_data);
    }

    unsigned DDSSourceData::getHeight() const
    {
        return TinyDDS_Height(m_reader->m_data);
    }

    unsigned DDSSourceData::getNumMipmaps() const
    {
        return std::max(1U, TinyDDS_NumberOfMipmaps(m_reader->m_data));
    }

    bool DDSSourceData::isCompressed() const
    {
        return TinyImageFormat_IsCompressed(getFormat());
    }

    TinyImageFormat DDSSourceData::getFormat() const
    {
        return TinyImageFormat_FromTinyDDSFormat(TinyDDS_GetFormat(m_reader->m_data));
    }

    void DDSSourceData::copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, std::span<DestTextureData> destination)
    {
        NAU_FATAL(m_reader);
        NAU_ASSERT(mipLevelStart < getNumMipmaps());

        std::lock_guard<std::mutex> guard(m_mutex);
        for (size_t i = 0; i < mipLevelsCount; ++i)
        {
            const uint32_t mip = static_cast<uint32_t>(mipLevelStart + i);
            DestTextureData& dest = destination[i];
            const auto [width, height] = TextureUtils::getMipSize(getWidth(), getHeight(), mip);
            const void* const data = TinyDDS_ImageRawData(m_reader->m_data, mip);
            TextureUtils::copyImageData(dest, width, height, getFormat(), reinterpret_cast<const std::byte*>(data));
        }
    }

}  // namespace nau
