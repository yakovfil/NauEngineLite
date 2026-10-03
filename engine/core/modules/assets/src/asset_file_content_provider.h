// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string_view>
#include <vector>

#include "nau/assets/asset_content_provider.h"

namespace nau
{
    class AssetFileContentProvider : public IAssetContentProvider
    {
        NAU_TYPEID(nau::AssetFileContentProvider)
        NAU_CLASS_BASE(IAssetContentProvider)

         Result<AssetContent> openStreamOrContainer(const AssetPath& assetPath) override;

         std::vector<std::string_view> getSupportedSchemes() const override;
    };
}  // namespace nau
