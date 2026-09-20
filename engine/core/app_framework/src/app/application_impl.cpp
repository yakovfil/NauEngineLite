// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./application_impl.h"

#include <cstdio>

#include "nau/diag/device_error.h"
#include "nau/service/internal/service_provider_initialization.h"
#include "nau/service/service_provider.h"
#ifndef NAU_MINIMAL_RUNTIME
    #include "nau/ui.h"
#endif

namespace nau
{
    ApplicationImpl::ApplicationImpl(RuntimeState::Ptr runtime) :
        m_runtime(std::move(runtime))
    {
        NAU_ASSERT(!applicationExists());
        setApplication(this);

        m_moduleManager->doModulesPhase(IModuleManager::ModulesPhase::Init);
        getServiceProvider().addService<MainLoopService>();
    }

    ApplicationImpl::~ApplicationImpl()
    {
        NAU_ASSERT(applicationExists());
        setApplication(nullptr);
    }

    Result<> cleanupFailedApplication(RuntimeState& runtime, IModuleManager::Ptr& modules)
    {
        auto previousExecutor = async::Executor::getThisThreadExecutor();
        auto queue = WorkQueue::create();
        async::Executor::setThisThreadExecutor(queue);
        auto task = getServiceProvider().as<core_detail::IServiceProviderInitialization&>().shutdownServices();
        while (!task.isReady())
        {
            queue->poll();
            std::this_thread::yield();
        }
        Result<> result = task.isRejected() ? Result<>{task.getError()} : Result<>{};
        task = nullptr;

        auto drain = runtime.shutdown(false);
        while (drain())
        {
            queue->poll();
            std::this_thread::yield();
        }
        setDefaultServiceProvider(nullptr);
        if (modules)
        {
            modules->doModulesPhase(IModuleManager::ModulesPhase::Cleanup);
            modules.reset();
        }
        async::Executor::setThisThreadExecutor(std::move(previousExecutor));
        queue.reset();
        runtime.completeShutdown();
        diag::setDeviceError(nullptr);
        return result;
    }

    Result<> ApplicationImpl::abortCreation()
    {
        m_appState = AppState::ShutdownCompleted;
        return cleanupFailedApplication(*m_runtime, m_moduleManager);
    }

    bool ApplicationImpl::isClosing() const
    {
        return m_stopRequested || getPhase() == ApplicationPhase::Stopping || getPhase() == ApplicationPhase::Stopped;
    }

    bool ApplicationImpl::hasExecutor()
    {
        return m_appWorkQueue != nullptr;
    }

    async::Executor::Ptr ApplicationImpl::getExecutor()
    {
        NAU_ASSERT(hasExecutor());
        return m_appWorkQueue;
    }

    void ApplicationImpl::shutdownCoreServices()
    {
        // 1. destroying services prior modules (because services are belongs to modules).
        setDefaultServiceProvider(nullptr);

        // 2. unload modules
        m_moduleManager->doModulesPhase(IModuleManager::ModulesPhase::Cleanup);
        m_moduleManager.reset();

        // 3. de-initialize diagnostics
        diag::setDeviceError(nullptr);
    }

    void ApplicationImpl::finishStartup()
    {
        ServiceProvider& serviceProvider = getServiceProvider();
        m_mainLoop = &serviceProvider.get<MainLoopService>();

#ifndef NAU_MINIMAL_RUNTIME
        if (getServiceProvider().has<ui::UiManager>())
        {
            m_uiManager = &getServiceProvider().get<ui::UiManager>();
        }

        if (getServiceProvider().has<vfx::VFXManager>())
        {
            m_vfxManager = &getServiceProvider().get<vfx::VFXManager>();
        }

#endif

        m_appState = AppState::Active;
    }

    void ApplicationImpl::beginStartup()
    {
        if (m_appState != AppState::Created)
        {
            return;
        }
        NAU_ASSERT(m_hostThreadId == std::thread::id{});

        m_hostThreadId = std::this_thread::get_id();
        m_appWorkQueue = WorkQueue::create();
        m_appWorkQueue->setName("App Work Queue");

        async::Executor::setThisThreadExecutor(m_appWorkQueue);

        if (m_stopRequested)
        {
            m_appState = AppState::ShutdownRequested;
            return;
        }
        m_appState = AppState::PreInitializing;
        getServiceProvider().as<core_detail::IServiceProviderInitialization&>().setInitializationStopToken(&m_stopRequested);
        m_startupTask = getServiceProvider().as<core_detail::IServiceProviderInitialization&>().preInitServices();
    }

    void ApplicationImpl::startupOnCurrentThread()
    {
        beginStartup();
        while (getPhase() == ApplicationPhase::PreInitializing || getPhase() == ApplicationPhase::Initializing)
        {
            pollLifecycle();
            std::this_thread::yield();
        }
        if (m_lifecycleError)
        {
            std::fprintf(stderr, "%s\n", m_lifecycleError->getDiagMessage().c_str());
        }
    }

    ApplicationPhase ApplicationImpl::getPhase() const
    {
        switch (m_appState.load())
        {
            case AppState::Created:
                return ApplicationPhase::Created;
            case AppState::PreInitializing:
                return ApplicationPhase::PreInitializing;
            case AppState::Initializing:
                return ApplicationPhase::Initializing;
            case AppState::Active:
                return ApplicationPhase::Running;
            case AppState::ShutdownCompleted:
                return ApplicationPhase::Stopped;
            default:
                return ApplicationPhase::Stopping;
        }
    }

    void ApplicationImpl::recordError(const async::Task<>& task, const char* phase)
    {
        if (task && task.isRejected())
        {
            eastl::string message = m_lifecycleError ? m_lifecycleError->getMessage() + "\n" : eastl::string{};
            message += phase;
            message += ": ";
            message += task.getError()->getDiagMessage();
            m_lifecycleError = NauMakeError(message);
        }
    }

    bool ApplicationImpl::isMainThread()
    {
        NAU_ASSERT(m_hostThreadId != std::thread::id{});

        return m_hostThreadId == std::this_thread::get_id();
    }

    Error::Ptr ApplicationImpl::getLifecycleError() const
    {
        if (m_lifecycleError)
            return m_lifecycleError;
        if (hasServiceProvider() && m_appState != AppState::Active)
        {
            return getServiceProvider().as<core_detail::IServiceProviderInitialization&>().getLifecycleError();
        }
        return nullptr;
    }

    bool ApplicationImpl::stepWithElapsedTime(std::chrono::milliseconds elapsed)
    {
        NAU_ASSERT(isMainThread());
        if (m_appState != AppState::Active || m_stopRequested)
            return false;
        mainGameStep(static_cast<float>(elapsed.count()) / 1000.f);
        return true;
    }

    uint64_t ApplicationImpl::getLifecycleProgress() const
    {
        return hasServiceProvider() ? getServiceProvider().as<core_detail::IServiceProviderInitialization&>().getLifecycleProgress() : 0;
    }

    bool ApplicationImpl::step()
    {
        const bool alive = pollLifecycle();
        if (alive && m_appState == AppState::Active && !m_stopRequested)
        {
            mainGameStep(m_tickStopwatch.tick());
        }
        return alive;
    }

    bool ApplicationImpl::pollLifecycle()
    {
        NAU_ASSERT(m_hostThreadId == std::this_thread::get_id(), "Invalid thread");
        if (m_appState == AppState::ShutdownCompleted)
        {
            return false;
        }

        m_appWorkQueue->poll();

        if (m_appState == AppState::PreInitializing || m_appState == AppState::Initializing)
        {
            if (!m_startupTask.isReady())
            {
                return true;
            }
            const bool preInit = m_appState == AppState::PreInitializing;
            recordError(m_startupTask, preInit ? "Pre-initialization" : "Initialization");
            m_startupTask = nullptr;
            if (m_lifecycleError || m_stopRequested)
            {
                m_appState = AppState::ShutdownRequested;
            }
            else if (preInit)
            {
                m_appState = AppState::Initializing;
                m_startupTask = getServiceProvider().as<core_detail::IServiceProviderInitialization&>().initServices();
            }
            else
            {
                finishStartup();
            }
        }
        if (m_appState == AppState::Active && m_stopRequested)
        {
            m_appState = AppState::ShutdownRequested;
        }
        if (m_appState == AppState::ShutdownRequested)
        {
            NAU_ASSERT(!m_shutdownTask);

            m_appState = AppState::GameShutdownProcessed;
            m_shutdownTask = m_mainLoop ? m_mainLoop->shutdownMainLoop() : async::makeResolvedTask();
        }
        else if (m_appState == AppState::GameShutdownProcessed)
        {
            NAU_FATAL(m_shutdownTask);
            if (!m_shutdownTask.isReady())
            {
                m_mainLoop->pollShutdown();
            }
            else
            {
                recordError(m_shutdownTask, "Game shutdown");
                m_shutdownTask = nullptr;
                m_shutdownTask = shutdownRuntime();
            }
        }
        else if (m_appState == AppState::RuntimeShutdownProcessed)
        {
            NAU_FATAL(m_shutdownTask);

            if (m_shutdownTask.isReady() && m_runtimeShutdown && !m_runtimeShutdown())
            {
                recordError(m_shutdownTask, "Service shutdown");
                m_shutdownTask = nullptr;
                completeShutdown();
            }
        }

        return m_appState != AppState::ShutdownCompleted;
    }

    void ApplicationImpl::stop()
    {
        m_stopRequested = true;
    }

    async::Task<> ApplicationImpl::shutdownRuntime()
    {
        [[maybe_unused]] const auto oldAppState = m_appState.exchange(AppState::RuntimeShutdownProcessed);
        NAU_ASSERT(oldAppState == AppState::GameShutdownProcessed);

        async::Task<> shutdownServicesTask = getServiceProvider().as<core_detail::IServiceProviderInitialization&>().shutdownServices();
        auto result = co_await shutdownServicesTask.doTry();
        m_runtimeShutdown = m_runtime->shutdown(false);
        co_await result;
    }

    void ApplicationImpl::completeShutdown()
    {
        [[maybe_unused]] const auto oldAppState = m_appState.exchange(AppState::ShutdownCompleted);
        NAU_ASSERT(oldAppState == AppState::RuntimeShutdownProcessed);

        shutdownCoreServices();
    }

    void ApplicationImpl::mainGameStep(float dt)
    {
        NAU_FATAL(m_mainLoop);

#ifndef NAU_MINIMAL_RUNTIME
        if (m_uiManager)
        {
            m_uiManager->update(dt);
        }

        if (m_vfxManager)
        {
            m_vfxManager->update(dt);
        }

#endif

        m_mainLoop->doGameStep(dt);
#if 0
        if (m_sceneManager != nullptr)
        {
            m_sceneManager->update(dt);

            if (imgui_get_state() != ImGuiState::OFF)
            {
                imgui_cache_render_data();
                imgui_update();
            }
        }
#endif
    }

}  // namespace nau
