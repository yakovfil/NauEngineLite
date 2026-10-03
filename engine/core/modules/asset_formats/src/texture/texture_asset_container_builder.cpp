// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./texture_asset_container_builder.h"

#include <vector>

#include "nau/assets/texture_asset_accessor.h"
#include "texture/texture_utils.h"
#include "tinydds.h"

namespace nau
{
    namespace
    {
        TinyDDS_WriteCallbacks ddsWriteCallbacks{
            [](void* user, char const* msg)
        {
            (void)msg;
        },
            [](void* user, size_t size)
        {
            return malloc(size);
        },
            [](void* user, void* memory)
        {
            free(memory);
        },
            [](void* user, const void* buffer, size_t byteCount)
        {
            io::IStreamWriter* const stream = reinterpret_cast<io::IStreamWriter*>(user);
            NAU_ASSERT(stream);
            stream->write(reinterpret_cast<const std::byte*>(buffer), byteCount).ignore();
        }};
    }  // namespace

    bool TextureAssetContainerBuilder::isAcceptable(nau::Ptr<> asset) const
    {
        NAU_FATAL(asset);
        return asset->is<ITextureAssetAccessor>();
    }

    Result<> TextureAssetContainerBuilder::writeAssetToStream(io::IStreamWriter::Ptr stream, nau::Ptr<> asset)
    {
        NAU_FATAL(asset);

        auto textureAssetAccessor = asset->as<ITextureAssetAccessor*>();
        NAU_FATAL(textureAssetAccessor);

        TextureDescription textureDescription = textureAssetAccessor->getDescription();

        const uint32_t channels = TinyImageFormat_ChannelCount(textureDescription.format);
        std::vector<uint32_t> dataSize(textureDescription.numMipmaps);
        std::vector<const void*> test(textureDescription.numMipmaps);
        std::vector<DestTextureData> dstData(textureDescription.numMipmaps);
        for (size_t i = 0; i < dstData.size(); ++i)
        {
            int width, height;
            DestTextureData& data = dstData[i];
            std::tie(width, height) = TextureUtils::getMipSize(textureDescription.width, textureDescription.height, i);
            const auto [srcRowPitch, srcSlicePitch] = *TextureUtils::getImagePitch(textureDescription.format, width, height);
            dataSize[i] = srcSlicePitch;

            data.rowPitch = srcRowPitch;
            data.slicePitch = srcSlicePitch;
            data.rowsCount = textureDescription.isCompressed ? std::max<size_t>(1, (height + 3) / 4) : height;
            data.outputBuffer = (uint32_t*)malloc(dataSize[i]);

            test[i] = data.outputBuffer;
        }

        textureAssetAccessor->copyTextureData(0, textureDescription.numMipmaps, dstData);
        TinyDDS_Format outDDSFormat = TinyImageFormat_ToTinyDDSFormat(textureDescription.format);
        bool result = TinyDDS_WriteImage(&ddsWriteCallbacks, stream.get(), textureDescription.width, textureDescription.height,
                                         1, 1, textureDescription.numMipmaps, outDDSFormat, false,
                                         false, dataSize.data(), test.data());

        for (size_t i = 0; i < dstData.size(); ++i)
        {
            free(dstData[i].outputBuffer);
        }
        return result ? ResultSuccess : NauMakeError("Fail to write dds");
    }
}  // namespace nau