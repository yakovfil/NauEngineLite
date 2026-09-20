// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./platform_window_service.h"

#include "nau/app/app_messages.h"
#include "nau/app/application.h"
#include "nau/app/application_services.h"
#include "nau/app/core_window_manager.h"
#include "nau/app/platform_window_status.h"
#include "nau/runtime/internal/runtime_object_registry.h"
#include "nau/service/service_provider.h"
#include "nau/threading/event.h"
#include "nau/threading/set_thread_name.h"

namespace nau
{
    PlatformWindowService::~PlatformWindowService()
    {
        NAU_ASSERT(!m_platformAppCompletedTask || m_platformAppCompletedTask.isReady());
    }

    async::Task<> PlatformWindowService::preInitService()
    {
        using namespace nau::async;

        auto platformAppClasses = getServiceProvider().findClasses<ICoreWindowManager>();
        if (platformAppClasses.empty())
        {
#ifdef __EMSCRIPTEN__
            NAU_LOG_ERROR("Browser PlatformApp module is missing");
            co_await Result<>{NauMakeError("Browser PlatformApp module is missing")};
#endif
            // LOG: NO Platform App module found
            co_return;  // return Error ?
        }

        auto& appClass = platformAppClasses.front();
        NAU_FATAL(appClass->getConstructor());

        nau::Ptr<> platformApp = rtti::TakeOwnership{(*appClass->getConstructor()->invoke(nullptr, {}))->as<IRefCounted*>()};
        NAU_FATAL(platformApp);

        TaskSource<> appReady;
        Task<> appReadyTask = appReady.getTask();

        m_platformAppThread = std::thread(
            [this](nau::Ptr<ICoreWindowManager> platformApp, async::TaskSource<> appReady)
        {
            threading::setThisThreadName("PlatformApp");

            TaskSource<> appCompleted;
            m_platformAppCompletedTask = appCompleted.getTask();
            scope_on_leave
            {
                appCompleted.resolve();
            };

            // Native binding may query its registered service during setup.
            const bool hasReadiness = platformApp->is<IPlatformWindowInitialization>();
            if (!hasReadiness)
                getServiceProvider().addService(platformApp);
            platformApp->bindToCurrentThread();
            if (auto* initialization = platformApp->as<IPlatformWindowInitialization*>())
            {
                auto ready = async::waitResult(initialization->windowReady());
                if (!ready)
                {
                    if (auto* disposable = platformApp->as<IDisposable*>())
                        disposable->dispose();
                    while (platformApp->pumpMessageQueue(false))
                    {
                    }
                    appReady.reject(ready.getError());
                    return;
                }
                getServiceProvider().addService(platformApp);
            }
            appReady.resolve();

            Result<> result;
            do
            {
                result = platformApp->pumpMessageQueue(true);
            } while (result);
        },
            std::move(platformApp), std::move(appReady));

        auto ready = co_await appReadyTask.doTry();
        if (!ready)
        {
            co_await m_platformAppCompletedTask;
            if (m_platformAppThread.joinable())
                m_platformAppThread.join();
            co_await ready;
        }

        m_messageSubscriptions.emplace_back(
            AppWindowClosed.subscribe(getBroadcaster(), []
        {
            getApplication().stop();
        }));
    }

    async::Task<> PlatformWindowService::initService()
    {
        return async::Task<>::makeResolved();
    }

    async::Task<> PlatformWindowService::disposeAsync()
    {
        auto* manager = getServiceProvider().find<ICoreWindowManager>();
        if (auto* const disposable = manager ? manager->as<IDisposable*>() : nullptr)
        {
            if (claimRuntimeDisposal(*disposable))
            {
                disposable->dispose();
            }
        }

        if (m_platformAppCompletedTask)
            co_await m_platformAppCompletedTask;

        if (m_platformAppThread.joinable())
        {
            m_platformAppThread.join();
        }
    }

    eastl::unique_ptr<IRttiObject> createPlatformWindowService()
    {
        return eastl::make_unique<PlatformWindowService>();
    }

}  // namespace nau
