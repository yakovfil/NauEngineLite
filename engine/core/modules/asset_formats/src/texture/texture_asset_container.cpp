// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "texture_asset_container.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "nau/service/service_provider.h"
#include "nau/string/string_conv.h"

namespace nau
{
    TextureAssetContainer::TextureAssetContainer(TextureSourceData textureData) :
        m_textureData(std::move(textureData))
    {
    }

    nau::Ptr<> TextureAssetContainer::getAsset([[maybe_unused]] std::string_view path)
    {
        return rtti::staticCast<IRefCounted*>(this);
    }

    std::vector<std::string> TextureAssetContainer::getContent() const
    {
        return {};
    }

    std::vector<std::string_view> TextureAssetContainerLoader::getSupportedAssetKind() const
    {
        return {"Texture/*", "png", "jpg", "hdr"};
    }

    TextureDescription TextureAssetContainer::getDescription() const
    {
        TextureDescription texDescription;
        texDescription.width = m_textureData.getWidth();
        texDescription.height = m_textureData.getHeight();
        texDescription.numMipmaps = m_textureData.getNumMipmaps();
        texDescription.format = m_textureData.getFormat();
        texDescription.isCompressed = m_textureData.isCompressed();

        return texDescription;
    }

    void TextureAssetContainer::copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, std::span<DestTextureData> destination)
    {
        NAU_ASSERT(destination.size() == mipLevelsCount);
        m_textureData.copyTextureData(mipLevelStart, mipLevelsCount, destination);
    }

    async::Task<IAssetContainer::Ptr> TextureAssetContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, AssetContentInfo info)
    {
        NAU_ASSERT(stream);
        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault())

        TinyImageFormat forceFormat = TinyImageFormat_UNDEFINED;
        if (info.kind == "hdr")
        {
            forceFormat = TinyImageFormat_R32G32B32A32_SFLOAT;
        }
        auto textureData = TextureSourceData::loadFromStream(stream, info.importSettings ? info.importSettings->as<RuntimeReadonlyDictionary *>() : getDefaultImportSettings(), forceFormat);
        if(!textureData)
        {
            co_return textureData.getError();
        }

        co_return rtti::createInstance<TextureAssetContainer>(std::move(*textureData));
    }

    RuntimeReadonlyDictionary::Ptr TextureAssetContainerLoader::getDefaultImportSettings() const
    {
        ImportSettings settings;
        return makeValueCopy(std::move(settings));
    }
}  // namespace nau
