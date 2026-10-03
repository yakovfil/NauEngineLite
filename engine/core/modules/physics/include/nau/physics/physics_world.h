// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "nau/debugRenderer/debug_render_system.h"
#include "nau/memory/standard_aliases.h"
#include "nau/physics/physics_defines.h"
#include "nau/physics/physics_material.h"
#include "nau/physics/physics_raycast.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/scene/scene_object.h"

namespace nau::physics
{
    class ICollisionShapesFactory;
    class IPhysicsBody;
    struct IPhysicsContactListener;
    struct PhysicsBodyCreationData;

    /**
     * @brief Provides physics system functionality like creating a physical body or casting a ray.
     */
    class NAU_ABSTRACT_TYPE IPhysicsWorld : public virtual IRefCounted
    {
        NAU_INTERFACE(nau::physics::IPhysicsWorld, IRefCounted)

        /**
         * @brief Pointer type used to reference IPhysicsWorld objects.
         */
        using Ptr = nau::Ptr<IPhysicsWorld>;

        /**
         * @brief Advances physics simulation for a single frame.
         *
         * @param [in] dt Delta time.
         */
        virtual void tick(float dt) = 0;

        /**
         * @brief Creates a physical body and places it in the physical world.
         *
         * @param [in] originSceneObjectUid Scene object's uid to attach the body to.
         * @param [in] creationData Keeps physical properties of the body assigned on its creation
         * @return                  A pointer to the created physical body.
         */
        virtual nau::Ptr<IPhysicsBody> createBody(Uid originSceneObjectUid, const PhysicsBodyCreationData& creationData) = 0;

        /**
         * @brief Allows or forbids contacts between to collision channels.
         *
         * @param [in] channelA, channelB   Channels to modify.
         * @param [in] collidable           Indicates whether the contact between **channelA** and **channelB** should be allowed.
         *
         * @note    Implementations should support commutativity, i.e. calling `setChannelsCollidable(a, b, true)` has to have similar effects to
         *          calling `setChannelsCollidable(b, a, true)`.
         *
         * See CollisionChannel for more information
         */
        virtual void setChannelsCollidable(CollisionChannel channelA, CollisionChannel channelB, bool collidable = true) = 0;

        /**
         * Clears all channels collision settings made by setChannelsCollidable().
         * Inter-channel collision settings of underlying physics system msut be reverted to the default state.
         */
        virtual void resetChannelsCollisionSettings() = 0;

        /**
         * @brief Assign a contact listener that is to receive messages about contacts between bodies in the physical world.
         *
         * @param [in] listener A pointer to the object to use as a listener.
         */
        virtual void setContactListener(nau::Ptr<IPhysicsContactListener> listener) = 0;

        /**
         * @brief Creates and registers a physical material.
         *
         * @param [in] name         Name to register the material under.
         * @param [in] friction     Degree of how the body resists being dragged. It has to be between 0.0 (no friction) and 1.0 (the body will stick to the surface and stay immobile). You can pass `std::nullopt` if you don't want the material to override body default friction.
         * @param [in] restitution  Degree of body tougheness on collision. It has to be between 0.0 (completely inelastic collision response) and 1.0 (completely elastic collision response). You can pass `std::nullopt` if you don't want the material to override body default restitution.
         * @return                  A pointer to the created material.
         */
        virtual IPhysicsMaterial::Ptr createMaterial(std::string_view name,
                                                     std::optional<TFloat> friction = std::nullopt,
                                                     std::optional<TFloat> restitution = std::nullopt);

        /**
         * @brief Allows to cast a ray between to points and check if it hits any physical bodies.
         *
         * @param [in] query    RayCasting properties.
         * @result              Hit data or `std::nullopt` is no hit has occurred.
         */
        virtual std::optional<RayCastResult> castRay(const nau::physics::RayCastQuery& query) const = 0;

        virtual async::Task<std::vector<RayCastResult>> castRaysAsync(std::vector<physics::RayCastQuery> queries) const = 0;

        inline async::Task<RayCastResult> castRayAsync(physics::RayCastQuery query) const
        {
            std::vector<RayCastResult> result = co_await this->castRaysAsync({std::move(query)});
            NAU_FATAL(!result.empty());
            co_return result.front();
        }

        virtual void drawDebug(nau::DebugRenderSystem& dr) {};

        virtual void setGravity(const nau::math::vec3& gravity) = 0;

    private:

        virtual void syncSceneState() = 0;

        friend class PhysicsWorldState;
    };
}  // namespace nau::physics
