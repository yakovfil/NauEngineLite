// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string_view>
#include <vector>

#include "nau/assets/asset_container.h"
#include "nau/assets/ui_asset_accessor.h"

namespace nau
{
    /**
     */
    class UiAssetContainerLoader final : public IAssetContainerLoader
    {
        NAU_INTERFACE(nau::UiAssetContainerLoader, IAssetContainerLoader)

    public:
        UiAssetContainerLoader() = default;

    private:
        std::vector<std::string_view> getSupportedAssetKind() const override;

        async::Task<IAssetContainer::Ptr> loadFromStream(io::IStreamReader::Ptr file, AssetContentInfo info) override;

        RuntimeReadonlyDictionary::Ptr getDefaultImportSettings() const override;
    };
}  // namespace nau
