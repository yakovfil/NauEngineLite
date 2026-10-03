// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <vector>

#include "nau/physics/components/rigid_body_component.h"
#include "nau/physics/physics_material.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/scene/scene_object.h"

namespace nau::physics
{
    /**
     * @brief Provides interface for accessing information about physical bodies contacts.
     */
    struct NAU_ABSTRACT_TYPE IPhysicsContactListener : public virtual IRefCounted
    {
        NAU_INTERFACE(nau::physics::IPhysicsContactListener, IRefCounted)

        /**
         * @brief Pointer type used to reference a IPhysicsContactListener objects.
         */
        using Ptr = Ptr<IPhysicsContactListener>;

        /**
         * @brief Encapsulates the data that is passed to the listener from each of the contacting bodies.
         */
        struct ContactManifold
        {
            /**
             * @brief Component which the contacting body is attached to.
             */
            // TODO: this should actually be a collider
            physics::RigidBodyComponent& rigidBody;

            /**
             * @brief Contacting body material.
             *
             * @note material is `NULL` when a ContactManifold instance is passed to onContactRemovedCompletely.
             */
            nau::physics::IPhysicsMaterial::Ptr material;
        };

        /**
         * @brief Called when a contact between two bodies begins.
         *
         * @param [in] data1, data2         Keep information about the contacting bodies.
         * @param [in] collisionWorldPoints Array of contact points (in world coordinates).
         */
        virtual void onContactAdded(const ContactManifold& data1, const ContactManifold& data2, const std::vector<math::vec3>& collisionWorldPoints) = 0;

        /**
         * @brief Called each frame while two bodies continue contacting.
         *
         * @param [in] data1, data2         Keep information about the contacting bodies.
         * @param [in] collisionWorldPoints Array of contact points (in world coordinates).
         */
        virtual void onContactContinued(const ContactManifold& data1, const ContactManifold& data2, const std::vector<math::vec3>& collisionWorldPoints) = 0;

        /**
         * @brief Called when a contact between two bodies ceases.
         *
         * @param [in] data1, data2         Keep information about the bodies that stopped contacting.
         */
        virtual void onContactRemovedCompletely(const ContactManifold& data1, const ContactManifold& data2) = 0;
    };
}  // namespace nau::physics
