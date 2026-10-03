// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/assets/asset_ref.h"

#include <string>
#include <string_view>

#include "nau/assets/asset_manager.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"

namespace nau
{
    AssetRefBase::~AssetRefBase() = default;

    AssetRefBase::AssetRefBase(AssetPath assetPath, bool lazyLoad) noexcept
    {
        if (!lazyLoad)
        {
            if (m_assetDescriptor = getServiceProvider().get<IAssetManager>().openAsset(assetPath); m_assetDescriptor)
            {
                m_assetDescriptor->load();
            }
        }
        else
        {
            m_assetDescriptor = getServiceProvider().get<IAssetManager>().preLoadAsset(assetPath);
        }
    }

    AssetRefBase::AssetRefBase(std::string_view assetPathStr, bool loadOnDemand) noexcept
        :
        AssetRefBase(AssetPath{assetPathStr}, loadOnDemand)
    {
    }

    AssetRefBase::AssetRefBase(IAssetDescriptor::Ptr assetDescriptor) noexcept :
        m_assetDescriptor(std::move(assetDescriptor))
    {
    }

    AssetRefBase::AssetRefBase(const AssetRefBase& other) noexcept = default;

    AssetRefBase::AssetRefBase(AssetRefBase&& other) noexcept = default;

    AssetRefBase& AssetRefBase::operator=(const AssetRefBase& other) noexcept = default;

    AssetRefBase& AssetRefBase::operator=(AssetRefBase&& other) noexcept = default;

    AssetRefBase::operator bool() const
    {
        return static_cast<bool>(m_assetDescriptor);
    }

    async::Task<IAssetView::Ptr> AssetRefBase::getAssetView(const AssetViewDescription& viewDescription)
    {
        if (!m_assetDescriptor)
        {
            return async::Task<IAssetView::Ptr>::makeResolved(nullptr);
        }

        return m_assetDescriptor->getAssetView(viewDescription);
    }

    async::Task<ReloadableAssetView::Ptr> AssetRefBase::getReloadableAssetView(const AssetViewDescription viewDescription)
    {
        if (!m_assetDescriptor)
        {
            return async::Task<ReloadableAssetView::Ptr>::makeResolved(nullptr);
        }

        return m_assetDescriptor->getReloadableAssetView(viewDescription);
    }

    Result<> parse(std::string_view fullAssetPath, AssetRefBase& assetRef)
    {
        if (strings::trim(fullAssetPath).empty())
        {
            assetRef = AssetRefBase{};
        }
        else if (AssetPath::isValid(strings::toStringView(fullAssetPath)))
        {
            assetRef = AssetRefBase{strings::toStringView(fullAssetPath)};
        }
        else
        {
            return NauMakeError("Invalid asset query");
        }

        return ResultSuccess;
    }

    std::string toString(const AssetRefBase& assetRef)
    {
        if (assetRef.m_assetDescriptor)
        {
            const AssetPath assetPath = assetRef.m_assetDescriptor->getAssetPath();
            const std::string pathStr = assetPath.toString();
            return std::string{pathStr.data(), pathStr.size()};
        }

        return {};
    }


}  // namespace nau
