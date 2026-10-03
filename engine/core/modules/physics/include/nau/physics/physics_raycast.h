// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <limits>
#include <vector>

#include "nau/math/math.h"
#include "nau/physics/physics_body.h"
#include "nau/physics/physics_defines.h"
#include "nau/scene/scene_object.h"

namespace nau::physics
{
    /**
     * @brief Encapsulates raycasting query settings.
     */
    struct RayCastQuery
    {
        uint32_t id = 0;

        /**
         * @brief World coordinates of the ray start.
         */
        math::vec3 origin;

        /**
         * @brief Direction of the ray.
         *
         * It is expected to be normalized.
         */
        math::vec3 direction;

        /**
         * @brief Length of the ray. Anything beyond this length will not be reported as a hit.
         */
        TFloat maxDistance = std::numeric_limits<TFloat>::max();

        /**
         * @brief List of channels the ray should hit.
         *
         * Empty list means the ray should hit any channel.
         */
        std::vector<CollisionChannel> reactChannels;

        /**
         * @brief Indicates whether the ray should ignore or hit triggers.
         */
        bool ignoreTriggers = false;

        /**
         * @brief Duration for which the ray should be visualized.
         *
         * @note Drawing rays only works in Debug mode.
         */
        float debugDrawDuration = {0};
    };

    /*
     * @brief Encapsulates ray hit information.
     */
    struct RayCastResult
    {
        uint32_t queryId = 0;

        /**
         * @brief Scene object @ref body is attached to.
         */
        // scene::ObjectWeakRef<scene::SceneObject> sceneObject;

        Uid sceneObjectUid = NullUid;

        /**
         * @brief Physical body that caused the hit.
         */
        // TODO: in future must be replaced with collider information
        scene::ObjectWeakRef<physics::RigidBodyComponent> rigidBody;

        /**
         * @brief Material of the @ref body collider sampled at the @ref position.
         */
        nau::physics::IPhysicsMaterial::Ptr material;

        /**
         * @brief Coordinates of the hit.
         */
        math::vec3 position;

        /**
         * @brief Normal vector to the hit body collider surface sampled at the @ref position.
         */
        math::vec3 normal;

        explicit operator bool() const
        {
            return static_cast<bool>(rigidBody) && (sceneObjectUid != NullUid);
        }

        bool hasTarget() const
        {
            return static_cast<bool>(*this);
        }
    };

}  // namespace nau::physics
