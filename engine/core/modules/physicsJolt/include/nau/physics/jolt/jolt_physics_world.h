// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <Jolt/Core/Color.h>
#include <Jolt/Core/Mutex.h>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include "nau/debugRenderer/debug_render_system.h"
#include "nau/math/math.h"
#include "nau/physics/jolt/jolt_debug_renderer.h"
#include "nau/physics/jolt/jolt_physics_body.h"
#include "nau/physics/jolt/jolt_physics_material.h"
#include "nau/physics/physics_contact_listener.h"
#include "nau/physics/physics_world.h"
#include "nau/rtti/rtti_impl.h"

namespace JPH
{
    class BodyInterface;
    class JobSystem;
    class PhysicsSystem;
    class ContactListener;
    class TempAllocatorImpl;
    class ObjectLayerPairFilterTable;
    class BroadPhaseLayerInterfaceTable;
    class ObjectVsBroadPhaseLayerFilter;
    class ObjectVsBroadPhaseLayerFilter;
} // namespace JPH

namespace nau::physics::jolt
{
    /**
     * @brief Implements nau::physics::IPhysicsWorld interface utilizing Jolt physics engine.
     */
    class JoltPhysicsWorld final : public IPhysicsWorld, public JPH::ContactListener 
    {
        NAU_CLASS(nau::physics::jolt::JoltPhysicsWorld, rtti::RCPolicy::Concurrent, IPhysicsWorld)

    public:

        /**
         * @brief Default constructor.
         * 
         * Initializes all associated Jolt objects.
         */
        JoltPhysicsWorld();

        /**
         * @brief Destructor.
         * 
         * Deallocates all memory.
         */
        ~JoltPhysicsWorld();

        /**
         * @brief Advances physics simulation for a single frame.
         *
         * @param [in] dt Delta time.
         */
        virtual void tick(float dt) override;

        /**
         * @brief Creates a physical body and places it in the physical world.
         *
         * @param [in] originObject Scene object to attach the body to.
         * @param [in] creationData Keeps physical properties of the body assigned on its creation
         * @return                  A pointer to the created physical body.
         */
        virtual nau::Ptr<IPhysicsBody> createBody(Uid originSceneObjectUid, const PhysicsBodyCreationData& creationData) override;

        /**
         * @brief Allows or forbids contacts between to collision channels.
         *
         * @param [in] channelA, channelB   Channels to modify.
         * @param [in] collidable           Indicates whether the contact benween **channelA** and **channelB** should be allowed.
         *
         * @note    Implementations should support commutativity, i.e. calling `setChannelsCollidable(a, b, true)` has to have similar effects to
         *          calling `setChannelsCollidable(b, a, true)`.
         *
         * See CollisionChannel for more information
         */
        virtual void setChannelsCollidable(CollisionChannel channelA, CollisionChannel channelB, bool collidable = true) override;
        virtual void resetChannelsCollisionSettings() override;

        /**
         * @brief Assign a contact listener that is to receive messages about contacts between bodies in the physical world.
         *
         * @param [in] listener A pointer to the object to use as a listener.
         */
        virtual void setContactListener(nau::Ptr<IPhysicsContactListener> listener) override;

        /**
         * @brief Called when a contact between two bodies begins.
         * 
         * @param [in]      body1, body2    Keep information about the contacting bodies.
         * @param [in]      manifold        Describes contact surface.
         * @param [in, out] settings        May be used to modify contact constraints.
         */
        virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2,
            const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override;

        /**
         * @brief Called each frame while two bodies continue contacting.
         *
         * @param [in]      body1, body2    Keep information about the contacting bodies.
         * @param [in]      manifold        Describes contact surface.
         * @param [in, out] settings        May be used to modify contact constraints.
         */
        virtual void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2,
            const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override;

        /**
         * @brief Called when a contact between two bodies ceases.
         *
         * @param [in] subShapePair         Keeps information about the bodies that stopped contacting.
         */
        virtual void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override;

        /**
         * @brief Provides access to the body interface.
         * 
         * @return A refernce to the body interface.
         * 
         * This interface allows to to create / remove bodies and to change their properties.
         */
        JPH::BodyInterface& getBodyInterface() const;

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
                                                     std::optional<TFloat> restitution = std::nullopt) override;

        /**
         * @brief Allows to cast a ray between to points and check if it hits any physical bodies.
         *
         * @param [in] query    Raycasting properties.
         * @result              Hit data or `std::nullopt` is no hit has occurred.
         */
        virtual std::optional<RayCastResult> castRay(const nau::physics::RayCastQuery& query) const override;

        async::Task<std::vector<RayCastResult>> castRaysAsync(std::vector<physics::RayCastQuery> queries) const override;

        /**
         * @brief Performs physics debug drawing.
         * 
         * @param [in] dr   Debug renderer that is responsible for physics debug drawing.
         * 
         * @note    Calling this effectively renders each physics body center of mass and collider outline
         *          given that their debug rendering is enabled (see nau::physics::jolt::JoltPhysicsBody::setDebugDrawEnabled).
         */
        virtual void drawDebug(nau::DebugRenderSystem& dr) override;

        virtual void setGravity(const nau::math::vec3& gravity) override;

        void syncSceneState() override;

    private:

        /**
         * @brief Retrieves friction, restitution and physical material of the body.
         * 
         * @param [in] body         Physical body to which the collision shape is attached.
         * @param [in] subShapeID   Index of the collision shape.
         * @return                  A tuple, containing friction, restitution and physical material of the collison shape.
         */
        static std::tuple<float, float, const JoltPhysicsMaterial*> getFrictionAndRestitution(const JPH::Body& body,
                                                                                              const JPH::SubShapeID& subShapeID);

        /**
         * @brief When called on contact, fills JPH::ContactSettings object with proper values.
         * 
         * @param [in]  body1, body2    Contacting bodies
         * @param [in]  manifold        Describes contact surface.
         * @param [out] settings        Output contact settings.
         */
        static void handleBodiesContact(const JPH::Body& body1, const JPH::Body& body2,
            const JPH::ContactManifold& manifold, JPH::ContactSettings& settings);

        /**
         * @brief Extracts contact points from the JPH::ContactManifold object.
         * 
         * @param [in] manifold Object to extract the points from.
         * @return A vector of contact points.
         */
        static std::vector<nau::math::vec3> calculateContactPoints(const JPH::ContactManifold& manifold);

        /**
         * @brief Sends a line segment to rendering.
         * 
         * @param [in] pos0, pos1   Segment tips.
         * @param [in] color        Color to apply to the line.
         * @param [in] time         Duration for which the line should remain on screen.
         */
        static void debugDrawLine(const nau::math::Point3& pos0, const nau::math::Point3& pos1,
            const nau::math::Color4& color, float time);

    private:
        enum ContactNotificationKind
        {
            Added,
            Continued,
            Removed
        };

        struct InternalContactManifold
        {
            Uid sceneObjectUid;
            IPhysicsMaterial::Ptr material;
        };

        struct InternalContactManifoldEntry
        {
            ContactNotificationKind kind;
            InternalContactManifold object1;
            InternalContactManifold object2;
            std::vector<math::vec3> collisionWorldPoints;
        };

        struct ContactData
        {
            const JoltPhysicsBody* body1 = nullptr;
            const JoltPhysicsBody* body2 = nullptr;
            std::set<JPH::SubShapeIDPair> contacts;
        };

        std::unique_ptr<JPH::ObjectLayerPairFilterTable> m_layerPairFilter; /** < Responsible for turning on or off collision between channels (layers).*/
        std::unique_ptr<JPH::BroadPhaseLayerInterfaceTable> m_broadPhaseLayerInterface;
        std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_objectOverBroadPhaseFilter;
        std::unique_ptr<JoltBodyDrawFilterImpl> m_bodyDrawFilter; /** < Responsible for deciding whether to draw a body or not within a debug draw call. */
        std::unique_ptr<DebugRendererImp> m_joltDebugRender;      /** < Performs physics debug drawing. */

        std::unique_ptr<JPH::PhysicsSystem> m_joltPhysicsSystem;
        std::unique_ptr<JPH::TempAllocatorImpl> m_joltTempAllocator;
        std::unique_ptr<JPH::JobSystem> m_joltJobSystem;

        IPhysicsContactListener::Ptr m_engineContactListener;
        IPhysicsMaterial::Ptr m_engineDefaultMaterial;

        int m_collisionStepsCount = 1; /** < Indicates granularity of collision detection stage within a physical world tick. */


        JPH::Mutex m_bodiesInContactGuard;

        /**
         * @brief Maps a pairs of Jolt ids of contacting bodies to the corresponding ContactData object.
         * 
         * We keep track bodies that are currently in contact by ourselves, for Jolt system contact listener
         * provide only bodyIDs of removed contacts. See JPH::ContactListener::OnContactRemoved for details.
         */
        std::map<std::pair<JPH::BodyID, JPH::BodyID>, ContactData> m_bodiesInContact;

        std::vector<InternalContactManifoldEntry> m_contactsData;
    };
} // namespace nau::physics::jolt

