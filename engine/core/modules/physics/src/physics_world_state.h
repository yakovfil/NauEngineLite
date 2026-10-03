// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <span>

#include "nau/memory/standard_aliases.h"
#include "nau/physics/components/rigid_body_component.h"
#include "nau/physics/physics_body.h"
#include "nau/physics/physics_world.h"
#include "nau/scene/scene_manager.h"
#include "nau/scene/scene_processor.h"

namespace nau::physics
{
    struct PhysicsBodyEntry
    {
        Uid componentUid;
        scene::ObjectWeakRef<RigidBodyComponent> componentRef;
        nau::Ptr<IPhysicsBody> physicsBody;

        PhysicsBodyEntry(const RigidBodyComponent& inRigidBodyComponent, nau::Ptr<IPhysicsBody> inPhysicsBody) :
            componentUid(inRigidBodyComponent.getUid()),
            componentRef(const_cast<RigidBodyComponent&>(inRigidBodyComponent)),
            physicsBody(std::move(inPhysicsBody))
        {
            NAU_FATAL(componentRef);
            NAU_FATAL(physicsBody);
        }
    };

    /**
     */
    class PhysicsWorldState
    {
    public:
        PhysicsWorldState(Uid worldUid);

        ~PhysicsWorldState();

        PhysicsWorldState(const PhysicsWorldState&) = delete;

        PhysicsWorldState& operator=(const PhysicsWorldState&) = delete;

        Uid getWorldUid() const;

        nau::Ptr<IPhysicsWorld> getPhysicsWorld() const;

        void tick(float secondsDt);

        async::Task<> activateComponents(std::span<const scene::Component*> components);

        void deactivateComponents(std::span<const scene::DeactivatedComponentData> components);

        bool syncSceneState(scene::ISceneManager& sceneManager);

    private:
        async::Task<nau::Ptr<IPhysicsBody>> createPhysicsBodyForComponent(const RigidBodyComponent& component);

        const Uid m_worldUid;
        nau::Ptr<IPhysicsWorld> m_physics;
        List<PhysicsBodyEntry> m_bodies;
        bool m_isPaused = false;
    };
}  // namespace nau::physics
