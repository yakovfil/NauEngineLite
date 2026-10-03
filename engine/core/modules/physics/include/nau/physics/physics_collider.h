// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <memory>
#include <vector>

#include "nau/math/transform.h"
#include "nau/physics/physics_defines.h"
#include "nau/physics/physics_material.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::physics
{
    /**
     * @brief Provides generic interface for collision shape access.
     */
    struct NAU_ABSTRACT_TYPE ICollisionShape : virtual IRttiObject
    {
        /**
         * @brief Pointer type used to reference a collision shape.
         */
        using Ptr = std::unique_ptr<ICollisionShape>;

        NAU_INTERFACE(nau::physics::ICollisionShape, IRttiObject)

        virtual void setShapeTransform(math::Transform localTransform) = 0;

        virtual math::Transform getShapeTransform() const = 0;
    };

    /**
     * @brief Provides generic interface for sphere collision shape access.
     */
    struct NAU_ABSTRACT_TYPE ISphereCollision : virtual ICollisionShape
    {
        NAU_INTERFACE(nau::physics::ISphereCollision, ICollisionShape)

        /**
         * @brief Retrieves the sphere radius.
         *
         * @return Radius of the sphere
         */
        virtual TFloat getRadius() const = 0;

        /**
         * @brief Sets the sphere radius.
         *
         * @param [in] radius   Radius value to assign.
         */
        virtual void setRadius(TFloat) = 0;
    };

    /**
     * @brief Provides generic interface for box collision shape access.
     */
    struct NAU_ABSTRACT_TYPE IBoxCollision : virtual ICollisionShape
    {
        NAU_INTERFACE(nau::physics::IBoxCollision, ICollisionShape)
    };

    /**
     * @brief Provides generic interface for capsule collision shape access.
     */
    struct NAU_ABSTRACT_TYPE ICapsuleCollision : virtual ICollisionShape
    {
        NAU_INTERFACE(nau::physics::ICapsuleCollision, ICollisionShape)

        /**
         * @brief Encapsulates capsule geometry data.
         */
        struct ConstructionData
        {
            /**
             * @brief Height of the cylindric part of the capsule shape.
             */
            TFloat height = 1.0f;
            TFloat radius = 0.5f;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(height),
                CLASS_FIELD(radius))
        };
    };

    /**
     * @brief Provides generic interface for cylinder collision shape access.
     */
    struct NAU_ABSTRACT_TYPE ICylinderCollision : virtual ICollisionShape
    {
        NAU_INTERFACE(nau::physics::ICylinderCollision, ICollisionShape)

        /**
         * @brief Encapsulates cylinder geometry data.
         */
        struct ConstructionData
        {
            TFloat height = 1.0f;
            TFloat radius = 0.5f;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(height),
                CLASS_FIELD(radius))
        };
    };

    /**
     * @brief Provides a generic interface for convex hull collision shape access.
     */
    struct NAU_ABSTRACT_TYPE IConvexHullCollision : virtual ICollisionShape
    {
        NAU_INTERFACE(nau::physics::IConvexHullCollision, ICollisionShape)

        /**
         * @brief Encapsulates convex hull geometry data.
         */
        struct ConstructionData
        {
            /**
             * @brief Vertices of the convex hull.
             */
            std::vector<nau::math::vec3> points;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(points))
        };
    };

    /**
     * @brief Provides a generic interface for mesh collision shape access.
     */
    struct NAU_ABSTRACT_TYPE IMeshCollision : virtual ICollisionShape
    {
        NAU_INTERFACE(nau::physics::IMeshCollision, ICollisionShape)

        /**
         * @brief Encapsulates mesh triangle geometry data.
         */
        struct Triangle
        {
            nau::math::vec3 p1;
            nau::math::vec3 p2;
            nau::math::vec3 p3;

            /**
             * Index of the triangle material in nau::physics::IMeshCollision::ConstructionData::materials.
             */
            uint32_t materialIndex{};

            NAU_CLASS_FIELDS(
                CLASS_FIELD(p1),
                CLASS_FIELD(p2),
                CLASS_FIELD(p3))
        };

        /**
         * @brief Encapsulates mesh geometry data.
         */
        struct ConstructionData
        {
            /**
             * @brief List of triangles making up the mesh.
             */
            std::vector<Triangle> triangles;

            /**
             * @brief List of materials which triangles in the mesh can reference via nau::physics::IMeshCollision::Triangle::materialIndex.
             */
            std::vector<nau::physics::IPhysicsMaterial*> materials;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(triangles))
        };
    };
}  // namespace nau::physics
