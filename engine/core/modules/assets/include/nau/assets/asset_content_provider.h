// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "nau/assets/asset_path.h"
#include "nau/io/fs_path.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/result.h"

namespace nau
{
    /**
     * @brief Encapsulates asset info.
     */
    struct AssetContentInfo
    {
        std::string kind;                  /** < Asset kind. It has to be a kind supported by the corresponding asset view container*/
        io::FsPath path;                   /** < Path to the asset file. */
        RuntimeObject::Ptr importSettings; /** < Settings to apply on load. */

        explicit operator bool() const
        {
            return !kind.empty();
        }
    };

    /**
     */
    class NAU_ABSTRACT_TYPE IAssetContentProvider
    {
        NAU_TYPEID(nau::IAssetContentProvider)

        using AssetContent = std::tuple<Ptr<>, AssetContentInfo>;

        virtual ~IAssetContentProvider() = default;

        virtual Result<AssetContent> openStreamOrContainer(const AssetPath& assetPath) = 0;

        virtual std::vector<std::string_view> getSupportedSchemes() const = 0;
    };

}  // namespace nau

