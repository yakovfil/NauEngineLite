// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <EASTL/variant.h>

#include "./asset_descriptor_impl.h"
#include "nau/assets/asset_container.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/assets/asset_listener.h"
#include "nau/assets/asset_manager.h"
#include "nau/assets/asset_path.h"
#include "nau/assets/asset_path_resolver.h"
#include "nau/assets/asset_view_factory.h"
#include "nau/async/multi_task_source.h"
#include "nau/rtti/rtti_impl.h"


namespace nau
{
    class IAssetContentProvider;

    /**
     */
    class AssetManagerImpl final : public IAssetManager,
                                   public IAssetDescriptorFactory
    {
        NAU_INTERFACE(nau::AssetManagerImpl, IAssetManager, IAssetDescriptorFactory)

    public:
        static AssetManagerImpl& getInstance();

        IAssetDescriptor::Ptr openAsset(const AssetPath& assetPath) override;
        IAssetDescriptor::Ptr preLoadAsset(const AssetPath& assetPath) override;

        IAssetDescriptor::Ptr findAsset(const AssetPath& assetPath) override;
        IAssetDescriptor::Ptr findAssetById(IAssetDescriptor::AssetId) override;

        void removeAsset(const AssetPath& assetPath) override;
        void unload(UnloadAssets flag) override;
        Result<AssetPath> resolvePath(const AssetPath& assetPath) override;

        IAssetDescriptor::Ptr createAssetDescriptor(IAssetContainer&, eastl::string_view innerPath) override;

        void addAssetContainer(const AssetPath& assetPath, IAssetContainer::Ptr) override;
        void removeAssetContainer(const AssetPath& assetPath) override;

        IAssetViewFactory* findAssetViewFactory(const rtti::TypeInfo& viewType);

        IAssetDescriptor::AssetId getNextAssetId();

        async::Task<> updateAssetView(IAssetDescriptor::AssetId assetId, const rtti::TypeInfo& viewType, IAssetView::Ptr oldAssetView, IAssetView::Ptr newAssetView);

    private:
        using SchemeHandler = eastl::variant<IAssetPathResolver*, IAssetContentProvider*>;

        struct ResolvedContentData
        {
            IAssetContentProvider* contentProvider = nullptr;
            AssetPath assetPath;
            AssetContentInfo contentInfo;

            explicit operator bool() const
            {
                return (contentProvider != nullptr) && assetPath;
            }
        };

        IAssetContainerLoader* findContainerLoader(const eastl::string& kind);

        /**
            @brief Resolves assetPath (which can be a virtual path) to actual (real) path.
            Then looks up content provider associated with the real (resolved) path;

            This method does require that m_mutex are locked by caller.
         */
        ResolvedContentData resolveAssetContent(const AssetPath& path);
        const eastl::vector<IAssetListener*>& getAssetListeners();

        eastl::unordered_map<AssetPath, nau::Ptr<AssetDescriptorImpl>> m_assets;
        eastl::unordered_map<eastl::string, IAssetContainerLoader*> m_containerLoaders;
        eastl::unordered_map<eastl::string_view, SchemeHandler> m_schemeHandlers;
        eastl::unordered_map<rtti::TypeIndex, IAssetViewFactory*> m_assetViewFactories;
        eastl::vector<IAssetListener*> m_assetListeners;

        std::atomic<IAssetDescriptor::AssetId> m_nextAssetId{1ULL};
        mutable std::shared_mutex m_mutex;
    };
}  // namespace nau
