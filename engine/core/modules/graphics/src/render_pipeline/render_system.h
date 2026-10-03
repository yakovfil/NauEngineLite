// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "nau/app/main_loop/game_system.h"
#include "nau/async/work_queue.h"
#include "nau/graphics/core_graphics.h"
#include "nau/memory/frame_allocator.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/scene/scene_processor.h"
#include "render/daBfg/bfg.h"
#include "render_scene.h"

namespace nau
{
    /**
     */
    class RenderSystem final : public ICoreGraphics,
                               public IGameSceneUpdate,
                               public scene::IComponentsAsyncActivator,
                               public IServiceInitialization,
                               public IServiceShutdown
    {
        NAU_RTTI_CLASS(RenderSystem,
                       ICoreGraphics,
                       IGameSceneUpdate,
                       scene::IComponentsAsyncActivator,
                       IServiceInitialization,
                       IServiceShutdown)

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(PreferredExecutionMode, ExecutionMode::Concurrent),
            CLASS_ATTRIBUTE(SceneAccessMode, SceneAccess::ReadOnly))

    public:
        using AsyncAction = Functor<async::Task<>()>;

        RenderSystem();
        ~RenderSystem() override;

        RenderScene::Ptr createScene(const std::string& sceneName);

        // std::shared_ptr<FrameGraph> createGraph();

#pragma region ServicePart
        async::Task<bool> renderFrame() override;

        async::Task<> activateComponentsAsync(Uid worldUid, std::span<const scene::Component*> components, async::Task<> barrier) override;

        async::Task<bool> update(std::chrono::milliseconds dt) override;

        std::optional<std::chrono::milliseconds> getFixedUpdateTimeStep() override
        {
            return std::nullopt;
        }

        void syncSceneState() override;

        async::Task<> preInitService() override;
        async::Task<> initService() override;
        async::Task<> shutdownService() override;

        async::Executor::Ptr getPreRenderExecutor();
        void addPreRenderJob(AsyncAction action);
#pragma endregion ServicePart

    protected:
        void renderMainScene();

        async::Task<> executeRenderJobs();
        void stopGraphics();

#pragma region RenderPart
        shaders::RenderState m_rendState;
        shaders::DriverRenderStateId m_drvRendStateId;
#pragma endregion RenderPart

        std::vector<dabfg::NodeHandle> m_nodeHandles;  // TODO: replace it to vector<FrameGraph> graphs when we make support for different frame-graphs
        std::map<std::string, RenderScene::Ptr> m_scenes;

#pragma region ServicePart
        nau::FrameAllocator m_frameAllocator;

        WorkQueue::Ptr m_preRenderWorkQueue = WorkQueue::create();
        std::mutex m_preRenderJobsMutex;
        std::vector<AsyncAction> m_preRenderJobs;

        std::atomic<bool> m_isDisposed{false};
        async::TaskSource<> m_renderStopedSignal;

        bool m_isInitialized = false;
#pragma endregion ServicePart
    };
}  // namespace nau
