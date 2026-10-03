// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "jolt_asset_factory.h"

#include <vector>

#include "nau/assets/mesh_asset_accessor.h"
#include "nau/physics/jolt/jolt_physics_assets.h"

namespace nau::physics::jolt
{
    std::vector<const rtti::TypeInfo*> JoltAssetFactory::getAssetViewTypes() const
    {
        return {
            &rtti::getTypeInfo<physics::ConvexHullAssetView>(),
            &rtti::getTypeInfo<physics::TriMeshAssetView>()};
    }

    async::Task<IAssetView::Ptr> JoltAssetFactory::createAssetView(nau::Ptr<> accessor, const rtti::TypeInfo& viewType)
    {
        if (viewType == rtti::getTypeInfo<physics::ConvexHullAssetView>())
        {
            auto& meshAccessor = accessor->as<IMeshAssetAccessor&>();
            co_return rtti::createInstance<JoltConvexHullAssetView>(meshAccessor);
        }
        else if (viewType == rtti::getTypeInfo<physics::TriMeshAssetView>())
        {
            auto& meshAccessor = accessor->as<IMeshAssetAccessor&>();
            co_return rtti::createInstance<JoltTriMeshAssetView>(meshAccessor);
        }
        else
        {
            NAU_FAILURE("Unknown physics asset view type");
        }

        co_return nullptr;
    }
}  // namespace nau::physics
