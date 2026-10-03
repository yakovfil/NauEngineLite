// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "../../include/graphics_assets/asset_view_factory.h"

#include <vector>

#include "../../include/graphics_assets/material_asset.h"
#include "../../include/graphics_assets/shader_asset.h"
#include "../../include/graphics_assets/skinned_mesh_asset.h"
#include "../../include/graphics_assets/static_mesh_asset.h"
#include "../../include/graphics_assets/texture_asset.h"

namespace nau
{
    std::vector<const rtti::TypeInfo*> GraphicsAssetViewFactory::getAssetViewTypes() const
    {
        return {
            &rtti::getTypeInfo<StaticMeshAssetView>(),
            &rtti::getTypeInfo<SkinnedMeshAssetView>(),
            &rtti::getTypeInfo<TextureAssetView>(),
            &rtti::getTypeInfo<ShaderAssetView>(),
            &rtti::getTypeInfo<MaterialAssetView>()
        };
    }

    async::Task<IAssetView::Ptr> GraphicsAssetViewFactory::createAssetView(nau::Ptr<> accessor, const rtti::TypeInfo& viewType)
    {
        if (viewType == rtti::getTypeInfo<StaticMeshAssetView>())
        {
            auto meshAssetView = co_await StaticMeshAssetView::createFromAssetAccessor(accessor);
            co_return meshAssetView;
        }
        if (viewType == rtti::getTypeInfo<SkinnedMeshAssetView>())
        {
            auto meshAssetView = co_await SkinnedMeshAssetView::createFromAssetAccessor(accessor);
            co_return meshAssetView;
        }
        if (viewType == rtti::getTypeInfo<TextureAssetView>())
        {
            auto textureAssetView = co_await TextureAssetView::createFromAssetAccessor(accessor);
            co_return textureAssetView;
        }
        if (viewType == rtti::getTypeInfo<ShaderAssetView>())
        {
            auto shaderAssetView = co_await ShaderAssetView::createFromAssetAccessor(accessor);
            co_return shaderAssetView;
        }
        if (viewType == rtti::getTypeInfo<MaterialAssetView>())
        {
            auto materialAssetView = co_await MaterialAssetView::createFromAssetAccessor(accessor);
            co_return materialAssetView;
        }

        NAU_FAILURE("Requests asset of unknown type ({})", viewType.getTypeName());

        co_return nullptr;
    }
}  // namespace nau
