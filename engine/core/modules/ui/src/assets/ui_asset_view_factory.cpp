// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "ui_asset_view_factory.h"

#include <vector>

#include "nau/ui/assets/ui_asset.h"

namespace nau::ui::data
{
    std::vector<const rtti::TypeInfo*> UiAssetViewFactory::getAssetViewTypes() const
    {
        return {
            &rtti::getTypeInfo<UiAssetView>()
        };
    }

    async::Task<IAssetView::Ptr> UiAssetViewFactory::createAssetView(nau::Ptr<> accessor, const rtti::TypeInfo& viewType)
    {
        if (viewType == rtti::getTypeInfo<UiAssetView>())
        {
            auto assetView = co_await UiAssetView::createFromAssetAccessor(accessor);
            co_return assetView;
        }
        
        NAU_FAILURE("Requests asset of unknown type ({})", viewType.getTypeName());

        co_return nullptr;
    }
} // namespace nau::ui::data