// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <concepts>
#include <memory>

#include "nau/physics/physics_assets.h"
#include "nau/physics/physics_collider.h"
#include "nau/physics/physics_defines.h"
#include "nau/physics/physics_material.h"
#include "nau/physics/physics_world.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/serialization/runtime_value_builder.h"

namespace nau::physics
{
    /**
     * @brief Provides interface for collider shape creation.
     */
    class NAU_ABSTRACT_TYPE ICollisionShapesFactory : public virtual IRefCounted
    {
        NAU_INTERFACE(nau::physics::ICollisionShapesFactory, IRefCounted)

        /**
         * @brief Creates a collider shape.
         *
         * @param [in] colliderType     Contains information about the actual collider type to create.
         * @param [in] constructionData Generic container for shape geometry.
         * @param [in] material         Material to apply to the collider.
         * @return                      A pointer to the created shape.
         */
        virtual ICollisionShape::Ptr createGenericCollisionShape(const rtti::TypeInfo& colliderType,
                                                                 const RuntimeValue::Ptr& constructionData = nullptr,
                                                                 nau::physics::IPhysicsMaterial* material = nullptr) const = 0;

        /**
         * @brief Creates a sphere collider shape.
         *
         * @param [in] radius           Sphere radius.
         * @param [in] material         Material to apply to the collider.
         * @return                      A pointer to the created sphere shape.
         */
        std::unique_ptr<ISphereCollision> createSphereCollision(TFloat radius, nau::physics::IPhysicsMaterial* material = nullptr) const;

        /**
         * @brief Creates a box collider shape.
         *
         * @param [in] extent           Box half-size.
         * @param [in] material         Material to apply to the collider.
         * @return                      A pointer to the created box shape.
         */
        std::unique_ptr<IBoxCollision> createBoxCollision(math::vec3 extent, nau::physics::IPhysicsMaterial* material = nullptr) const;

        /**
         * @brief Creates a capsule collider shape.
         *
         * @param [in] constructionData Capsule geometry.
         * @param [in] material         Material to apply to the collider.
         * @return                      A pointer to the created capsule shape.
         */
        std::unique_ptr<ICapsuleCollision> createCapsuleCollision(
            const ICapsuleCollision::ConstructionData& constructionData,
            nau::physics::IPhysicsMaterial* material = nullptr) const;

        /**
         * @brief Creates a cylinder collider shape.
         *
         * @param [in] constructionData Cylinder geometry.
         * @param [in] material         Material to apply to the collider.
         * @return                      A pointer to the created cylinder shape.
         */
        std::unique_ptr<ICylinderCollision> createCylinderCollision(
            const ICylinderCollision::ConstructionData& constructionData,
            nau::physics::IPhysicsMaterial* material = nullptr) const;

        /**
         * @brief Creates a convex hull collider shape.
         *
         * @param [in] constructionData Convex hull geometry.
         * @param [in] material         Material to apply to the collider.
         * @return                      A pointer to the created convex hull shape.
         */
         [[deprecated("Prefer createConvexHullCollision from asset")]]
        virtual IConvexHullCollision::Ptr createConvexHullCollision(
            const IConvexHullCollision::ConstructionData& constructionData,
            nau::physics::IPhysicsMaterial* material = nullptr) const = 0;

        /**
         * @brief Creates a mesh collider shape.
         *
         * @param [in] constructionData Mesh geometry.
         * @return                      A pointer to the created mesh shape.
         */
        [[deprecated("Prefer createMeshCollision from asset")]]
        virtual IMeshCollision::Ptr createMeshCollision(const IMeshCollision::ConstructionData& constructionData) const = 0;

        virtual ICollisionShape::Ptr createMeshCollisionFromAsset(nau::Ptr<physics::TriMeshAssetView> asset) const = 0;

        virtual ICollisionShape::Ptr createConvexHullCollisionFromAsset(nau::Ptr<physics::ConvexHullAssetView> asset) const = 0;


    private:
        /**
         * @brief Creates a collider shape.
         *
         * @tparam CollisionShapeType   Output shape type. In has to be a subclass of ICollisionShape.
         * @tparam Argument             Type of shape geometry data.
         *
         * @param [in] constructionData Generic container for shape geometry.
         * @param [in] material         A pointer to the material to apply to the collider.
         * @return                      A pointer to the created shape.
         */
        template <std::derived_from<ICollisionShape> CollisionShapeType, typename Argument>
        std::unique_ptr<CollisionShapeType> createGenericCollisionShapeTyped(
            const Argument& constructionData,
            nau::physics::IPhysicsMaterial* material = nullptr) const;
    };

    template <std::derived_from<ICollisionShape> CollisionShapeType, typename Argument>
    std::unique_ptr<CollisionShapeType> ICollisionShapesFactory::createGenericCollisionShapeTyped(
        const Argument& constructionData,
        nau::physics::IPhysicsMaterial* material) const
    {
        static_assert(nau::HasRuntimeValueRepresentation<Argument>, "Argument has no dynamic representation");
        const RuntimeValue::Ptr constructorValue = makeValueRef(constructionData /* using stack allocator*/);

        ICollisionShape::Ptr shape = createGenericCollisionShape(rtti::getTypeInfo<CollisionShapeType>(), constructorValue, material);
        NAU_ASSERT(shape);

        if (!shape)
        {
            return nullptr;
        }

        return rtti::pointer_cast<CollisionShapeType>(std::move(shape));
    }

    inline std::unique_ptr<ISphereCollision> ICollisionShapesFactory::createSphereCollision(TFloat radius, physics::IPhysicsMaterial* material) const
    {
        return createGenericCollisionShapeTyped<ISphereCollision>(radius, material);
    }

    inline std::unique_ptr<IBoxCollision> ICollisionShapesFactory::createBoxCollision(math::vec3 extent, physics::IPhysicsMaterial* material) const
    {
        return createGenericCollisionShapeTyped<IBoxCollision>(extent, material);
    }

    inline std::unique_ptr<ICapsuleCollision> ICollisionShapesFactory::createCapsuleCollision(const ICapsuleCollision::ConstructionData& constructionData, physics::IPhysicsMaterial* material) const
    {
        return createGenericCollisionShapeTyped<ICapsuleCollision>(constructionData, material);
    }

    inline std::unique_ptr<ICylinderCollision> ICollisionShapesFactory::createCylinderCollision(const ICylinderCollision::ConstructionData& constructionData, physics::IPhysicsMaterial* material) const
    {
        return createGenericCollisionShapeTyped<ICylinderCollision>(constructionData, material);
    }

}  // namespace nau::physics
