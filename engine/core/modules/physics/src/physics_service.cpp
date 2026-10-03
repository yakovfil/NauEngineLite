// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "physics_service.h"

#include <algorithm>
#include <optional>
#include <span>

#include "nau/app/application.h"
#include "nau/physics/components/rigid_body_component.h"
#include "nau/physics/physics_collision_shapes_factory.h"
#include "nau/scene/internal/scene_manager_internal.h"
#include "nau/scene/scene_manager.h"
#include "nau/scene/world.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"
#include "nau/threading/set_thread_name.h"

namespace nau::physics
{
    namespace
    {
        template <class TService>
        auto createServiceClass() -> nau::Ptr<TService>
        {
            auto classes = getServiceProvider().findClasses<TService>();
            if (classes.empty())
            {
                return nullptr;
            }

            const IMethodInfo* const ctor = classes.front()->getConstructor();
            NAU_FATAL(ctor);

            return ctor->invokeToPtr(nullptr, {});
        }

    }  // namespace

    PhysicsService::PhysicsService() = default;

    async::Task<> PhysicsService::preInitService()
    {
        if (auto shapesFactory = createServiceClass<ICollisionShapesFactory>())
        {
            getServiceProvider().addService(shapesFactory);
        }
        else
        {
            NAU_FAILURE("no ICollisionShapesFactory implementation found");
            co_yield NauMakeError("no ICollisionShapesFactory implementation found");
        }

        co_return;
    }

    async::Task<> PhysicsService::initService()
    {
        scene::ISceneManager& sceneManager = getServiceProvider().get<scene::ISceneManager>();
        m_physicsWorlds.emplace_back(sceneManager.getDefaultWorld().getUid());

        co_return;
    }

    async::Task<> PhysicsService::shutdownService()
    {
        if (!m_isShutdownRequested.exchange(true))
        {
            NAU_LOG_DEBUG("Physics shutdown started");
            co_await m_physicsStopedSignal.getTask();
            NAU_LOG_DEBUG("Physics shutdown completed");
        }
    }

    async::Task<> PhysicsService::activateComponentsAsync(Uid worldUid, std::span<const scene::Component*> components, async::Task<> barrier)
    {
        if (!scene::hasAcceptableComponent<RigidBodyComponent>(components))
        {
            co_return;
        }

        co_await m_preUpdateWorkQueue;

        PhysicsWorldState* const physWorld = getPhysicsWorldState(worldUid, true);
        NAU_FATAL(physWorld);
        co_await physWorld->activateComponents(components);
    }

    async::Task<> PhysicsService::deactivateComponentsAsync(Uid worldUid, std::span<const scene::DeactivatedComponentData> components)
    {
        co_await m_preUpdateWorkQueue;

        if (PhysicsWorldState* const physWorld = getPhysicsWorldState(worldUid, false))
        {
            physWorld->deactivateComponents(components);
        }
    }

    async::Task<bool> PhysicsService::update(std::chrono::milliseconds dt)
    {
        m_preUpdateWorkQueue->poll();

        if (m_isShutdownRequested)
        {
            m_physicsWorlds.clear();
            m_physicsStopedSignal.resolve();
            co_return false;
        }

        constexpr float MaxSimulationStep = 0.1f;
        const float simulationTimeStep = std::min(static_cast<float>(dt.count()) / 1000.f, MaxSimulationStep);

        for (PhysicsWorldState& physWorld : m_physicsWorlds)
        {
            physWorld.tick(simulationTimeStep);
        }

        co_return true;
    }

    std::optional<std::chrono::milliseconds> PhysicsService::getFixedUpdateTimeStep()
    {
        // The target refresh rate value can be calculated more intelligently (or at least loaded from global settings)
        constexpr size_t TargetStepsPerSecond = 75;
        constexpr size_t SimulationStepTime = 1000.0f / TargetStepsPerSecond;

        return std::chrono::milliseconds(SimulationStepTime);
    }

    void PhysicsService::syncSceneState()
    {
        using namespace nau::scene;

        if (m_isShutdownRequested)
        {
            return;
        }


        ISceneManager& sceneManager = getServiceProvider().get<ISceneManager>();
        for (PhysicsWorldState& physWorld : m_physicsWorlds)
        {
            physWorld.syncSceneState(sceneManager);
        }
    }

    async::Executor::Ptr PhysicsService::getExecutor()
    {
        return m_preUpdateWorkQueue;
    }

    nau::Ptr<IPhysicsWorld> PhysicsService::findPhysicsWorld(Uid worldUid)
    {
        if (worldUid == NullUid)
        {
            worldUid = getServiceProvider().get<scene::ISceneManager>().getDefaultWorld().getUid();
        }

        PhysicsWorldState* const worldState = getPhysicsWorldState(worldUid, false);
        return worldState ? worldState->getPhysicsWorld() : nullptr;
    }

    PhysicsWorldState* PhysicsService::getPhysicsWorldState(Uid worldUid, bool createOnDemand)
    {
        auto iter = std::find_if(m_physicsWorlds.begin(), m_physicsWorlds.end(), [&worldUid](const PhysicsWorldState& state)
        {
            return state.getWorldUid() == worldUid;
        });

        if (iter != m_physicsWorlds.end())
        {
            return &(*iter);
        }
        else if (createOnDemand)
        {
            m_physicsWorlds.emplace_back(worldUid);
            return &m_physicsWorlds.back();
        }

        return nullptr;
    }
}  // namespace nau::physics
