// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <string_view>

#include "nau/assets/asset_descriptor.h"
#include "nau/assets/asset_path.h"
#include "nau/rtti/rtti_object.h"
#include "nau/utils/typed_flag.h"

namespace nau
{
    enum class UnloadAssets
    {
        OnlyUnused = NauFlag(1)
    };

    NAU_DEFINE_TYPED_FLAG(UnloadAssets)

    /**
     */
    struct NAU_ABSTRACT_TYPE IAssetManager
    {
        NAU_TYPEID(nau::IAssetManager)

        /**
         */
        virtual IAssetDescriptor::Ptr openAsset(const AssetPath& assetPath) = 0;

        /*
        */
        virtual IAssetDescriptor::Ptr preLoadAsset(const AssetPath& assetPath) = 0;

        /**
         */
        virtual IAssetDescriptor::Ptr findAssetById(IAssetDescriptor::AssetId) = 0;
        
        /**
         */
        virtual IAssetDescriptor::Ptr findAsset(const AssetPath& assetPath) = 0;

        /**
         */
        virtual void removeAsset(const AssetPath& assetPath) = 0;

        /**
         */
        virtual void unload(UnloadAssets flag = UnloadAssets::OnlyUnused) = 0;

        /**
         */
        virtual Result<AssetPath> resolvePath(const AssetPath& assetPath) = 0;

    };

}  // namespace nau
