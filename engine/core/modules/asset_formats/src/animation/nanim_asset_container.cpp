// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nanim_asset_container.h"

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "nanim_animation_accessor.h"

namespace nau
{
    std::vector<std::string_view> NanimAssetContainerLoader::getSupportedAssetKind() const
    {
        return { "nanim" };
    }

    async::Task<IAssetContainer::Ptr> NanimAssetContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, [[maybe_unused]] AssetContentInfo)
    {
        auto container = rtti::createInstance<NanimStreamAssetContainer>(std::move(stream));
        co_return container;
    }

    RuntimeReadonlyDictionary::Ptr NanimAssetContainerLoader::getDefaultImportSettings() const
    {
        return nullptr;
    }

    NanimStreamAssetContainer::NanimStreamAssetContainer(io::IStreamReader::Ptr stream)
    {
        m_stream = stream;
    }

    io::IStreamReader::Ptr NanimStreamAssetContainer::getStream()
    {
        return m_stream;
    }

    nau::Ptr<> NanimStreamAssetContainer::getAsset(std::string_view path)
    {
        if (m_assetInstance == nullptr)
        {
            m_assetInstance = rtti::createInstance<NanimAnimationAssetAccessor>(*this);
        }
        return m_assetInstance;
    }

    std::vector<std::string> NanimStreamAssetContainer::getContent() const
    {
        return {};
    }

}  // namespace nau
