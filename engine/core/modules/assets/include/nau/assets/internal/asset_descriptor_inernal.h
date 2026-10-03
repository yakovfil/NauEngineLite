// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>

#include "nau/assets/asset_view.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/typed_flag.h"

namespace nau::assets
{
    /**
     */
    struct AssetInternalState
    {
        uint64_t assetId = 0;
        IAssetView::Ptr view;
        nau::Ptr<> accessor;
    };

    /**
     */
    enum class InternalStateOpts
    {
        Default = 0,
        Accessor = NauFlag(1)
    };

    NAU_DEFINE_TYPED_FLAG(InternalStateOpts)

    /**
     */
    struct NAU_ABSTRACT_TYPE IAssetDescriptorInternal
    {
        NAU_TYPEID(nau::assets::IAssetDescriptorInternal)

        virtual std::optional<AssetInternalState> getCachedAssetViewInternalState(const rtti::TypeInfo* viewType, InternalStateOptsFlag) = 0;
    };

}  // namespace nau::assets
