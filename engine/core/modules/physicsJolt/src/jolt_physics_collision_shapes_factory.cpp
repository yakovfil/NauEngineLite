// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "jolt_physics_collision_shapes_factory.h"

#include <memory>

#include "nau/physics/jolt/jolt_physics_collider.h"
#include "nau/physics/jolt/jolt_physics_material.h"

namespace nau::physics::jolt
{
    ICollisionShape::Ptr JoltPhysicsCollisionShapesFactory::createGenericCollisionShape(const rtti::TypeInfo& colliderType,
        const RuntimeValue::Ptr& constructionData, nau::physics::IPhysicsMaterial* material) const
    {
        JPH::PhysicsMaterial* joltMaterial = nullptr;
        if (material)
        {
            joltMaterial = material->as<NauJoltPhysicsMaterialImpl*>()->joltMaterial();
        }

        if (colliderType == rtti::getTypeInfo<ISphereCollision>())
        {
            const TFloat radius = constructionData ? *runtimeValueCast<TFloat>(constructionData) : 1.0f;
            return std::make_unique<JoltSphereCollision>(radius, joltMaterial);
        }
        else if (colliderType == rtti::getTypeInfo<IBoxCollision>())
        {
            const math::vec3 extent = constructionData ? *runtimeValueCast<math::vec3>(constructionData) : math::vec3::one();
            return std::make_unique<JoltBoxCollision>(extent, joltMaterial);
        }
        else if (colliderType == rtti::getTypeInfo<ICapsuleCollision>())
        {
            const auto data = constructionData ? *runtimeValueCast<ICapsuleCollision::ConstructionData>(constructionData) : ICapsuleCollision::ConstructionData{.height = 1.f, .radius = 0.5f};
            return std::make_unique<JoltCapsuleCollision>(data, joltMaterial);
        }
        else if (colliderType == rtti::getTypeInfo<ICylinderCollision>())
        {
            const auto data = constructionData ? *runtimeValueCast<ICylinderCollision::ConstructionData>(constructionData) : ICylinderCollision::ConstructionData{.height = 1.f, .radius = 0.5f};
            return std::make_unique<JoltCylinderCollision>(data, joltMaterial);
        }

        NAU_LOG_WARNING("Unknown collision shape type: ({})", colliderType.getTypeName());
        return nullptr;
    }

    IConvexHullCollision::Ptr JoltPhysicsCollisionShapesFactory::createConvexHullCollision(const IConvexHullCollision::ConstructionData& data,
        nau::physics::IPhysicsMaterial* material) const
    {
        return std::make_unique<JoltConvexHullCollision>(data, material
                                                                   ? material->as<NauJoltPhysicsMaterialImpl*>()->joltMaterial()
                                                                   : nullptr);
    }

    IMeshCollision::Ptr JoltPhysicsCollisionShapesFactory::createMeshCollision(const IMeshCollision::ConstructionData& data) const
    {
        return std::make_unique<JoltMeshCollision>(data);
    }

    ICollisionShape::Ptr JoltPhysicsCollisionShapesFactory::createMeshCollisionFromAsset(nau::Ptr<physics::TriMeshAssetView> asset) const
    {
        return std::make_unique<JoltMeshCollision>(std::move(asset));
    }

    ICollisionShape::Ptr JoltPhysicsCollisionShapesFactory::createConvexHullCollisionFromAsset(nau::Ptr<physics::ConvexHullAssetView> asset) const
    {
        return std::make_unique<JoltConvexHullCollision>(std::move(asset));
    }
}  // namespace nau::physics::jolt
