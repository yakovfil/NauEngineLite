// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <vector>

#include "nau/assets/asset_view_factory.h"

namespace nau::animation::data
{
    class AnimationAssetViewFactory final : public IAssetViewFactory
    {
        NAU_RTTI_CLASS(nau::animation::data::AnimationAssetViewFactory, IAssetViewFactory)
    public:
        AnimationAssetViewFactory() = default;

    private:
        std::vector<const rtti::TypeInfo*> getAssetViewTypes() const override;

        async::Task<IAssetView::Ptr> createAssetView(nau::Ptr<> accessor, const rtti::TypeInfo& viewType) override;
    };
}  // namespace nau::animation::data
