// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include <memory>

#include "nau/physics/jolt/jolt_physics_assets.h"
#include "nau/physics/physics_collider.h"
#include "nau/physics/physics_defines.h"
#include "nau/physics/physics_material.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::physics::jolt
{

    /**
     * @brief Jolt collision shape class wrapper.
     */
    class JoltCollisionShape
    {
        NAU_TYPEID(nau::physics::jolt::JoltCollisionShape)
    public:

        /**
         * @brief Retrieves the Jolt collision shape object.
         * 
         * @return Jolt collision shape object.
         */
        JPH::RefConst<JPH::Shape> getCollisionShape() const;

    protected:

        /**
         * @brief Default constructor.
         */
        JoltCollisionShape();

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] shape    Jolt collision shape instance.
         */
        JoltCollisionShape(JPH::RefConst<JPH::Shape> shape);

        /**
         * @brief Binds Jolt collision shape to the object.
         * 
         * @param [in] shape Shape to bind.
         */
        void setCollisionShape(JPH::RefConst<JPH::Shape> shape);

        math::Transform m_transform;

    private:

        /**
         * @brief Wrapped Jolt collision shape object.
         */
        JPH::RefConst<JPH::Shape> m_collisionShape;
    };

    template <std::derived_from<ICollisionShape> ShapeType>
    class JoltCollisionShapeImpl : public JoltCollisionShape,
                                   public ShapeType
    {
    protected:
        using JoltCollisionShape::JoltCollisionShape;

    public:
        NAU_TYPEID(JoltCollisionShapeImpl)
        NAU_CLASS_BASE(JoltCollisionShape, ShapeType)

        void setShapeTransform(math::Transform localTransform) final
        {
            m_transform = localTransform;
        }

        math::Transform getShapeTransform() const final
        {
            return m_transform;
        }
    };

    /**
     * @brief Implements nau::physics::ISphereCollision interface utilizing Jolt physics engine.
     */
    class JoltSphereCollision final : public JoltCollisionShapeImpl<ISphereCollision>
    {
        using Base = JoltCollisionShapeImpl<ISphereCollision>;
        NAU_RTTI_CLASS(nau::physics::jolt::JoltSphereCollision, Base)

    public:

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] radius   Sphere radius.
         * @param [in] material A pointer to the collision shape material.
         */
        JoltSphereCollision(TFloat radius, JPH::PhysicsMaterial* material = nullptr);

        /**
         * @brief Retrieves radius of the sphere.
         * 
         * @return Sphere radius.
         */
        TFloat getRadius() const override;

        /**
         * @brief Changes radius of the sphere.
         * 
         * @param [in] radius   Value to assign.
         */
        void setRadius(TFloat) override;
    };

    /**
     * @brief Implements nau::physics::IBoxCollision interface utilizing Jolt physics engine.
     */
    class JoltBoxCollision final : public JoltCollisionShapeImpl<IBoxCollision>
    {
        using Base = JoltCollisionShapeImpl<IBoxCollision>;
        NAU_RTTI_CLASS(nau::physics::jolt::JoltBoxCollision, Base)

    public:

        /**
         * @brief Initialization constructor.
         *
         * @param [in] radius   Box half-size.
         * @param [in] material A pointer to the collision shape material.
         */
        JoltBoxCollision(math::vec3 extent, JPH::PhysicsMaterial* material = nullptr);
    };

    /**
     * @brief Implements nau::physics::ICapsuleCollision interface utilizing Jolt physics engine.
     */
    class JoltCapsuleCollision final : public JoltCollisionShapeImpl<ICapsuleCollision>
    {
        using Base = JoltCollisionShapeImpl<ICapsuleCollision>;
        NAU_RTTI_CLASS(nau::physics::jolt::JoltCapsuleCollision, Base)

    public:

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] constructionData Capsule geometry.
         * @param [in] material         A pointer to the collision shape material.
         */
        JoltCapsuleCollision(const ConstructionData& constructionData, JPH::PhysicsMaterial* material = nullptr);
    };

    /**
     * @brief Implements nau::physics::ICylinderCollision interface utilizing Jolt physics engine.
     */
    class JoltCylinderCollision final : public JoltCollisionShapeImpl<ICylinderCollision>
    {
        using Base = JoltCollisionShapeImpl<ICylinderCollision>;
        NAU_RTTI_CLASS(nau::physics::jolt::JoltCylinderCollision, Base)

    public:


        /**
         * @brief Initialization constructor.
         *
         * @param [in] constructionData Cylinder geometry.
         * @param [in] material         A pointer to the collision shape material.
         */
        JoltCylinderCollision(const ConstructionData& constructionData, JPH::PhysicsMaterial* material = nullptr);
    };

    /**
     * @brief Implements nau::physics::IConvexHullCollision interface utilizing Jolt physics engine.
     */
    class JoltConvexHullCollision final : public JoltCollisionShapeImpl<IConvexHullCollision>
    {
        using Base = JoltCollisionShapeImpl<IConvexHullCollision>;
        NAU_RTTI_CLASS(nau::physics::jolt::JoltConvexHullCollision, Base)

    public:


        /**
         * @brief Initialization constructor.
         *
         * @param [in] constructionData Convex hull geometry.
         * @param [in] material         A pointer to the collision shape material.
         */
        JoltConvexHullCollision(const ConstructionData& constructionData, JPH::PhysicsMaterial* material = nullptr);
        JoltConvexHullCollision(nau::Ptr<JoltConvexHullAssetView> convexHullAssetView);

    private:
        nau::Ptr<JoltConvexHullAssetView> m_convexHullAsset;
    };

    /**
     * @brief Implements nau::physics::IMeshCollision interface utilizing Jolt physics engine.
     */
    class JoltMeshCollision final : public JoltCollisionShapeImpl<IMeshCollision>
    {
        using Base = JoltCollisionShapeImpl<IMeshCollision>;
        NAU_RTTI_CLASS(nau::physics::jolt::JoltMeshCollision, Base)

    public:


        /**
         * @brief Initialization constructor.
         *
         * @param [in] constructionData Mesh geometry.
         * @param [in] material         A pointer to the collision shape material.
         */
        JoltMeshCollision(const ConstructionData& constructionData);
        JoltMeshCollision(nau::Ptr<JoltTriMeshAssetView> meshAssetView);

    private:
        nau::Ptr<JoltTriMeshAssetView> m_meshAsset;
    };
}  // namespace nau::physics::jolt
