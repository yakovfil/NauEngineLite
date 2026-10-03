// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "asset_view_factory.h"

#include <vector>

#include "nau/animation/assets/animation_asset.h"
#include "nau/animation/assets/skeleton_asset.h"

namespace nau::animation::data
{
    std::vector<const rtti::TypeInfo*> AnimationAssetViewFactory::getAssetViewTypes() const
    {
        return {
            &rtti::getTypeInfo<AnimationAssetView>(),
            &rtti::getTypeInfo<SkeletonAssetView>()
        };
    }

    async::Task<IAssetView::Ptr> AnimationAssetViewFactory::createAssetView(nau::Ptr<> accessor, const rtti::TypeInfo& viewType)
    {
        if(viewType == rtti::getTypeInfo<AnimationAssetView>())
        {
            auto assetView = co_await AnimationAssetView::createFromAssetAccessor(accessor);
            co_return assetView;
        }
        if (viewType == rtti::getTypeInfo<SkeletonAssetView>())
        {
            auto skeletonAssetView = co_await SkeletonAssetView::createFromAssetAccessor(accessor);
            co_return skeletonAssetView;
        }

        NAU_FAILURE("Requests asset of unknown type ({})", viewType.getTypeName());

        co_return nullptr;
    }
}  // namespace nau::animation::data
