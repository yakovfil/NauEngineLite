// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "nau/assets/asset_content_provider.h"
#include "nau/assets/asset_path.h"
#include "nau/rtti/type_info.h"


namespace nau
{
    /**
     */
    class NAU_ABSTRACT_TYPE IAssetPathResolver
    {
        NAU_TYPEID(nau::IAssetPathResolver)

        virtual ~IAssetPathResolver() = default;

        virtual std::tuple<AssetPath, AssetContentInfo> resolvePath(const AssetPath& assetPath) = 0;

        virtual std::vector<std::string_view> getSupportedSchemes() const = 0;
    };
}  // namespace nau
