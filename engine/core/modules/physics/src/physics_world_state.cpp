// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "physics_world_state.h"

#include <algorithm>
#include <span>

#include "nau/assets/asset_manager.h"
#include "nau/physics/physics_assets.h"
#include "nau/physics/physics_collision_shapes_factory.h"
#include "nau/service/service_provider.h"

namespace nau::physics
{
    PhysicsWorldState::PhysicsWorldState(Uid worldUid) :
        m_worldUid(worldUid)
    {
        auto classes = getServiceProvider().findClasses<IPhysicsWorld>();
        NAU_FATAL(classes.size() == 1);
        const IMethodInfo* const ctor = classes.front()->getConstructor();
        NAU_FATAL(ctor);
        m_physics = ctor->invokeToPtr(nullptr, {});
        NAU_FATAL(m_physics);
    }

    PhysicsWorldState::~PhysicsWorldState()
    {
        m_bodies.clear();
    }

    Uid PhysicsWorldState::getWorldUid() const
    {
        return m_worldUid;
    }

    nau::Ptr<IPhysicsWorld> PhysicsWorldState::getPhysicsWorld() const
    {
        NAU_FATAL(m_physics);
        return m_physics;
    }

    void PhysicsWorldState::tick(float secondsDt)
    {
        if (m_isPaused)
        {
            return;
        }

        m_physics->tick(secondsDt);
    }

    async::Task<> PhysicsWorldState::activateComponents(std::span<const scene::Component*> components)
    {
        using namespace nau::scene;

        for (const Component* const component : components)
        {
            auto* const rigidBodyComponent = component->as<const RigidBodyComponent*>();
            if (rigidBodyComponent)
            {
                nau::Ptr<IPhysicsBody> physBody = co_await createPhysicsBodyForComponent(*rigidBodyComponent);
                if (!physBody)
                {
                    NAU_LOG_ERROR("Fail to create PhysicsBody for corresponding RigidBodyComponent");
                    continue;
                }

                m_bodies.emplace_back(*rigidBodyComponent, std::move(physBody));
            }
        }
    }

    void PhysicsWorldState::deactivateComponents(std::span<const scene::DeactivatedComponentData> components)
    {
        std::erase_if(m_bodies, [&components](const PhysicsBodyEntry& entry)
        {
            return std::any_of(components.begin(), components.end(), [&entry](const scene::DeactivatedComponentData& c)
            {
                return c.componentUid == entry.componentUid;
            });
        });
    }

    bool PhysicsWorldState::syncSceneState(scene::ISceneManager& sceneManager)
    {
        using namespace nau::scene;

        IWorld::WeakRef world = sceneManager.findWorld(m_worldUid);
        if (!world)
        {
            return false;
        }

        if (m_isPaused != sceneManager.getDefaultWorld().isSimulationPaused())
        {
            m_isPaused = !m_isPaused;
            if (m_isPaused)
            {
                NAU_LOG_WARNING(
                    "Physics: the simulation has just been DISABLED. In this mode, "
                    "the physics world is not updated, but is synchronized with the scene.");
            }
            else
            {
                NAU_LOG_WARNING(
                    "Physics: the simulation has just been ENABLED. In this mode, "
                    "the physics world is not synchronized with the scene, but the scene is updated from it.");
            }
        }

        for (auto entry = m_bodies.begin(); entry != m_bodies.end();)
        {
            if (!entry->componentRef)
            {
                entry = m_bodies.erase(entry);
                continue;
            }

            NAU_FATAL(entry->physicsBody);

            SceneObject& parentObject = entry->componentRef->getParentObject();

            if (!m_isPaused)
            {
                math::mat4 physTransform;
                entry->physicsBody->getTransform(physTransform);

                // TODO: maybe do not need to getting world transform ?
                // If no need to deal with scale
                auto transform = parentObject.getWorldTransform();
                transform.setTranslation(physTransform.getTranslation());
                transform.setRotation(math::quat(physTransform.getUpper3x3()));
                parentObject.setWorldTransform(transform);

                entry->componentRef->applyPhysicsBodyActions(entry->physicsBody.get());
            }
            else
            {
                entry->physicsBody->setTransform({parentObject.getRotation(), parentObject.getWorldTransform().getTranslation()});
                entry->componentRef->applyPhysicsBodyActions(nullptr);
            }

            ++entry;
        }

        if (!m_isPaused)
        {
            m_physics->syncSceneState();
        }

        return true;
    }

    async::Task<nau::Ptr<IPhysicsBody>> PhysicsWorldState::createPhysicsBodyForComponent(const RigidBodyComponent& component)
    {
        auto& shapeFactory = getServiceProvider().get<physics::ICollisionShapesFactory>();

        ICollisionShape::Ptr collisionShape;

        if (auto meshCollision = component.getMeshCollision())
        {
            if (component.useConvexHullForCollision())
            {
                auto convexHullAssetView = co_await meshCollision.getAssetViewTyped<physics::ConvexHullAssetView>();
                if (convexHullAssetView)
                {
                    collisionShape = shapeFactory.createConvexHullCollisionFromAsset(std::move(convexHullAssetView));
                }
            }
            else
            {
                auto meshAssetView = co_await meshCollision.getAssetViewTyped<physics::TriMeshAssetView>();
                if (meshAssetView)
                {
                    collisionShape = shapeFactory.createMeshCollisionFromAsset(std::move(meshAssetView));
                }
            }
        }
        else
        {
            // TODO: currently support form compound collision shapes is not implemented.
            const auto& collisions = component.getCollisions();
            if (!collisions.spheres.empty())
            {
                collisionShape = shapeFactory.createSphereCollision(collisions.spheres.front().radius);
            }
            else if (!collisions.boxes.empty())
            {
                collisionShape = shapeFactory.createBoxCollision(collisions.boxes.front().extent);
            }
            else if (!collisions.capsules.empty())
            {
                const auto& c = collisions.capsules.front();
                collisionShape = shapeFactory.createCapsuleCollision({.height = c.height, .radius = c.radius});
            }
            else if (!collisions.cylinders.empty())
            {
                const auto& c = collisions.cylinders.front();
                collisionShape = shapeFactory.createCylinderCollision({.height = c.height, .radius = c.radius});
            }
        }

        if (auto scale = component.getWorldTransform().getScale(); collisionShape && !scale.similar(math::vec3::one()))
        {
            math::Transform shapeTransform;
            shapeTransform.setScale(scale);
            collisionShape->setShapeTransform(shapeTransform);
        }

        if (!collisionShape)
        {
            NAU_LOG_ERROR("Can not create collisions for rigid body ({})", component.getParentObject().getName());
            co_return nullptr;
        }

        const scene::SceneObject& parentObject = component.getParentObject();

        PhysicsBodyCreationData creationData{
            .collisionShape = std::move(collisionShape),
        };

        creationData.mass = component.getMass();
        creationData.collisionChannel = component.getCollisionChannel();
        creationData.motionType = component.getMotionType();
        creationData.position = parentObject.getWorldTransform().getTranslation();
        creationData.rotation = parentObject.getRotation();
        creationData.isTrigger = component.isTrigger();
        creationData.debugDraw = component.isDebugDrawEnabled();
        creationData.comOffset = component.centerMassShift();

        co_return m_physics->createBody(parentObject.getUid(), creationData);
    }

}  // namespace nau::physics
