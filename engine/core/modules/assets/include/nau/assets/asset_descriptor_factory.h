// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string_view>

#include "nau/assets/asset_descriptor.h"
#include "nau/assets/asset_path.h"
#include "nau/io/fs_path.h"
#include "nau/rtti/type_info.h"


namespace nau
{
    struct IAssetContainer;

    /**
     */
    struct NAU_ABSTRACT_TYPE IAssetDescriptorFactory
    {
        NAU_TYPEID(IAssetDescriptorFactory)

        /**
         */
        virtual IAssetDescriptor::Ptr createAssetDescriptor(IAssetContainer& container, std::string_view innerPath) = 0;

        virtual void addAssetContainer(const AssetPath& assetPath, nau::Ptr<IAssetContainer> container) = 0;

        virtual void removeAssetContainer(const AssetPath& assetPath) = 0;
    };

}  // namespace nau
