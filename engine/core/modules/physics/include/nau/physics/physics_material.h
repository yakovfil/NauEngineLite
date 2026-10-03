// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>
#include <string_view>

#include "nau/physics/physics_defines.h"
#include "nau/rtti/rtti_impl.h"

namespace nau::physics
{
    /**
     * @brief Provides interface for managing physical materials.
     * 
     * Materials can be used to override physical properties (such as friction, restitution) 
     * of an area (sub-shape, triangle) of a collider surface or the whole collider surface.
     * Material data can be obtained at a contact point and later utilized to choose a sound or a particle effect to visualize the contact.
     */
    class NAU_ABSTRACT_TYPE IPhysicsMaterial : public virtual IRefCounted
    {
        NAU_INTERFACE(nau::physics::IPhysicsMaterial, IRefCounted)

        /**
         * @brief A pointer type for referencing IPhysicsMaterial objects.
         */
        using Ptr = Ptr<IPhysicsMaterial>;

        /**
         * @brief Retrives the name of the material.
         * 
         * @return Name of the material.
         */
        virtual std::string_view getName() const = 0;

        /**
         * @brief Retrieves the friction value.
         * 
         * @return Friction value. An implementation may return an empty value, if this material is not intended to override the body default friction.
         * 
         * See nau::physics::PhysicsBodyCreationData::friction for more information.
         */
        virtual std::optional<TFloat> getFriction() const = 0;

        /**
         * @brief Retrieves the restitution value.
         *
         * @return Restitution value. An implementation may return an empty value, if this material is not intended to override the body default restitution.
         * 
         * See nau::physics::PhysicsBodyCreationData::restitution for more information.
         */
        virtual std::optional<TFloat> getRestitution() const = 0;
    };

}  // namespace nau::physics
