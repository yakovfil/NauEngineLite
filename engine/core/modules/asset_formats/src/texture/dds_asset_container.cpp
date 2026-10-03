// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "dds_asset_container.h"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "nau/service/service_provider.h"

namespace nau
{
    DDSAssetContainer::DDSAssetContainer(DDSSourceData textureData) :
        m_textureData(std::move(textureData))
    {
    }

    nau::Ptr<> DDSAssetContainer::getAsset([[maybe_unused]] std::string_view path)
    {
        return rtti::staticCast<IRefCounted*>(this);
    }

    std::vector<std::string> DDSAssetContainer::getContent() const
    {
        return {};
    }

    TextureDescription DDSAssetContainer::getDescription() const
    {
        TextureDescription texDescription;
        texDescription.width = m_textureData.getWidth();
        texDescription.height = m_textureData.getHeight();
        texDescription.numMipmaps = m_textureData.getNumMipmaps();
        texDescription.format = m_textureData.getFormat();
        texDescription.arraySize = m_textureData.getArraySize();
        texDescription.depth = m_textureData.getDepth();
        texDescription.type = m_textureData.getType();
        texDescription.isCompressed = m_textureData.isCompressed();

        return texDescription;
    }

    void DDSAssetContainer::copyTextureData(size_t mipLevelStart, size_t mipLevelsCount, std::span<DestTextureData> destination)
    {
        NAU_ASSERT(destination.size() == mipLevelsCount);
        m_textureData.copyTextureData(mipLevelStart, mipLevelsCount, destination);
    }

    std::vector<std::string_view> DDSAssetContainerLoader::getSupportedAssetKind() const
    {
        return {"texture/dds", "dds"};
    }

    async::Task<IAssetContainer::Ptr> DDSAssetContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, [[maybe_unused]] AssetContentInfo)
    {
        NAU_ASSERT(stream);
        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault())

        auto textureData = DDSSourceData::loadFromStream(stream);
        if(!textureData)
        {
            co_return textureData.getError();
        }

        co_return rtti::createInstance<DDSAssetContainer>(std::move(*textureData));
    }

    RuntimeReadonlyDictionary::Ptr DDSAssetContainerLoader::getDefaultImportSettings() const
    {
        return nullptr;
    }
}  // namespace nau
