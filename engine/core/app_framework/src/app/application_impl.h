// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./logging_service.h"
#include "./main_loop/main_loop_service.h"
#include "nau/app/application.h"
#include "nau/app/application_lifecycle.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/async/work_queue.h"
#include "nau/memory/singleton_memop.h"
#include "nau/messaging/messaging.h"
#include "nau/module/module_manager.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/internal/runtime_state.h"
#ifndef NAU_MINIMAL_RUNTIME
    #include "nau/scene/internal/scene_manager_internal.h"
#endif
#include "nau/utils/stopwatch.h"
#ifndef NAU_MINIMAL_RUNTIME
    #include "nau/vfx_manager.h"
#endif

namespace nau
{

    namespace ui
    {
        struct UiManager;
    }

    /**
     */
    class ApplicationImpl final : public Application,
                                  public ApplicationLifecycle
    {
        NAU_RTTI_CLASS(nau::ApplicationImpl, Application, ApplicationLifecycle)
        NAU_DECLARE_SINGLETON_MEMOP(ApplicationImpl)

    public:
        explicit ApplicationImpl(RuntimeState::Ptr runtime);

        Result<> abortCreation();

        void beginStartup() override;
        bool pollLifecycle() override;
        ApplicationPhase getPhase() const override;
        Error::Ptr getLifecycleError() const override;
        uint64_t getLifecycleProgress() const override;
        bool stepWithElapsedTime(std::chrono::milliseconds elapsed) override;

        ~ApplicationImpl();

        void startupOnCurrentThread() override;
        bool isMainThread() override;

        bool step() override;

        void stop() override;

        bool isClosing() const override;
        async::Executor::Ptr getExecutor() override;
        bool hasExecutor() override;

    private:
        enum class AppState
        {
            Created,
            PreInitializing,
            Initializing,
            Active,
            ShutdownRequested,
            GameShutdownProcessed,
            RuntimeShutdownProcessed,
            ShutdownCompleted
        };

        void shutdownCoreServices();

        void finishStartup();

        void recordError(const async::Task<>& task, const char* phase);

        async::Task<> shutdownRuntime();

        void completeShutdown();

        void mainGameStep(float dt);

        RuntimeState::Ptr m_runtime;
        IModuleManager::Ptr m_moduleManager = createModuleManager();
        WorkQueue::Ptr m_appWorkQueue;

        std::thread::id m_hostThreadId;
        std::atomic<AppState> m_appState = AppState::Created;
        std::atomic<bool> m_stopRequested = false;
        async::Task<> m_startupTask;
        Error::Ptr m_lifecycleError;

        MainLoopService* m_mainLoop = nullptr;

#ifndef NAU_MINIMAL_RUNTIME
        ui::UiManager* m_uiManager = nullptr;
        vfx::VFXManager* m_vfxManager = nullptr;
#endif

        async::Task<> m_shutdownTask;
        Functor<bool()> m_runtimeShutdown;
        nau::TickStopwatch m_tickStopwatch;
    };

    Result<> cleanupFailedApplication(RuntimeState& runtime, IModuleManager::Ptr& modules);
}  // namespace nau
