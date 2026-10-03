// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>
#include <span>

#include "nau/app/main_loop/game_system.h"
#include "nau/async/work_queue.h"
#include "nau/memory/standard_aliases.h"
#include "nau/physics/components/rigid_body_component.h"
#include "nau/physics/core_physics.h"
#include "nau/physics/internal/core_physics_internal.h"
#include "nau/physics/physics_world.h"
#include "nau/runtime/async_disposable.h"
#include "nau/scene/scene_processor.h"
#include "physics_world_state.h"

namespace nau::physics
{
    class PhysicsService final : public ICorePhysics,
                                 public ICorePhysicsInternal,
                                 public IServiceInitialization,
                                 public IServiceShutdown,
                                 public scene::IComponentsAsyncActivator,
                                 public IGameSceneUpdate

    {
        NAU_RTTI_CLASS(nau::physics::PhysicsService,
                       ICorePhysics,
                       ICorePhysicsInternal,
                       IServiceInitialization,
                       IServiceShutdown,
                       scene::IComponentsAsyncActivator,
                       IGameSceneUpdate)

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(PreferredExecutionMode, ExecutionMode::Concurrent),
            CLASS_ATTRIBUTE(SceneAccessMode, SceneAccess::Modify))

    public:
        PhysicsService();

    private:
        async::Task<> preInitService() override;

        async::Task<> initService() override;

        async::Task<> shutdownService() override;

        async::Task<> activateComponentsAsync(Uid, std::span<const scene::Component*> components, async::Task<> barrier) override;

        async::Task<> deactivateComponentsAsync(Uid, std::span<const scene::DeactivatedComponentData> components) override;

        async::Task<bool> update(std::chrono::milliseconds dt) override;

        std::optional<std::chrono::milliseconds> getFixedUpdateTimeStep() override;

        void syncSceneState() override;

        nau::Ptr<IPhysicsWorld> findPhysicsWorld(Uid worldUid) override;

        async::Executor::Ptr getExecutor() override;

        PhysicsWorldState* getPhysicsWorldState(Uid uid, bool createOnDemand);

        std::atomic<bool> m_isPaused = false;
        std::atomic<bool> m_isShutdownRequested = false;
        async::TaskSource<> m_physicsStopedSignal;
        List<PhysicsWorldState> m_physicsWorlds;
        WorkQueue::Ptr m_preUpdateWorkQueue = WorkQueue::create();
    };

}  // namespace nau::physics
