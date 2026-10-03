// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <concepts>
#include <string>

#include "nau/assets/asset_path.h"
#include "nau/assets/asset_view.h"
#include "nau/async/task.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "reloadable_asset_view.h"

namespace nau
{
    /**
     *
     */
    struct AssetViewDescription
    {
        const rtti::TypeInfo* viewApi = nullptr;

        AssetViewDescription() = default;

        AssetViewDescription(const rtti::TypeInfo& api) :
            viewApi(&api)
        {
        }
    };

    /**
     * @brief Provides interface for retrieving asset views.
     */
    struct NAU_ABSTRACT_TYPE IAssetDescriptor : virtual IRefCounted
    {
        NAU_INTERFACE(nau::IAssetDescriptor, IRefCounted)

        using Ptr = nau::Ptr<IAssetDescriptor>;
        using AssetId = uint64_t;

        enum class UnloadResult
        {
            Unloaded,
            UnloadedHasReferences
        };

        enum class LoadState
        {
            None,
            InProgress,
            Ready
        };

        virtual AssetId getAssetId() const = 0;

        virtual AssetPath getAssetPath() const = 0;

        /**
         * @brief Schedules asset view retrieval.
         *
         * @param [in] assetViewDesc    Description of the asset view to retrieve (first of all, its type).
         * @return                      Task object providing the operation status as well as a pointer to the retrieved view.
         */
        virtual async::Task<IAssetView::Ptr> getAssetView(const AssetViewDescription& = {}) = 0;
        virtual async::Task<ReloadableAssetView::Ptr> getReloadableAssetView(const AssetViewDescription& = {}) = 0;

        virtual async::Task<nau::Ptr<>> getRawAsset() = 0;

        /**
         * @brief Requests asset load.
         *
         * Current implementation supports asset load at the first getAssetView invokation.
         * However, a 'pre-load' can be requested using @ref load method.
         */
        virtual void load() = 0;

        /**
         * @brief Clears resource cache.
         *
         * @return Unloading result.
         */
        virtual UnloadResult unload() = 0;

        /**
         * @brief Retrieves asset loading operation status.
         *
         * @return Loading status value.
         */
        virtual LoadState getLoadState() const = 0;

        /**
         * @brief Schedules asset view retrieval.
         *
         * @tparam ViewType Type of the asset view to retrieve. It has to be a subclass of IAssetView.
         *
         * @return Task object providing the operation status as well as a pointer to the retrieved view.
         */
        template <std::derived_from<IAssetView> ViewType>
        async::Task<nau::Ptr<ViewType>> getAssetViewTyped();
    };

    template <std::derived_from<IAssetView> ViewType>
    async::Task<nau::Ptr<ViewType>> IAssetDescriptor::getAssetViewTyped()
    {
        const AssetViewDescription description{
            rtti::getTypeInfo<ViewType>()};

        IAssetView::Ptr assetView = co_await getAssetView(description);
        if (!assetView)
        {
            co_return nullptr;
        }

        if (!assetView->is<ViewType>())
        {
            co_return nullptr;
        }

        co_return assetView;
    }

}  // namespace nau
