// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <emscripten.h>
#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "nau/app/application_services.h"
#include "nau/app/core_window_manager.h"
#include "nau/app/platform_window_status.h"
#include "nau/diag/log_subscribers.h"
#include "nau/diag/logging.h"
#include "nau/messaging/messaging.h"
#include "nau/module/module_manager.h"
#include "nau/runtime/async_disposable.h"
#include "nau/runtime/internal/runtime_state.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

using namespace nau;
using namespace std::chrono_literals;

TEST(BrowserPlatform, ProductionService)
{
    auto runtime = RuntimeState::create();
    diag::setLogger(diag::createLogger());
    auto logSubscription = diag::getLogger().subscribe(diag::createConioOutputLogSubscriber());
    setDefaultServiceProvider(createServiceProvider());
    getServiceProvider().addService(AsyncMessageSource::create());
    auto modules = createModuleManager();
    // clang-format off
    const bool missingBackend = MAIN_THREAD_EM_ASM_INT({ return Module['scenario'] === 'backend'; });
    // clang-format on
    if (!missingBackend)
        modules->doModulesPhase(IModuleManager::ModulesPhase::Init);
    EXPECT_EQ(getServiceProvider().findClasses<ICoreWindowManager>().size(), missingBackend ? 0u : 1u);
    auto service = createPlatformWindowService();
    auto ready = async::waitResult(service->as<IServiceInitialization&>().preInitService());
    // clang-format off
    const bool expectFailure = MAIN_THREAD_EM_ASM_INT({ return Module['expectFailure'] ? 1 : 0; });
    // clang-format on
    EXPECT_EQ(static_cast<bool>(ready), !expectFailure);
    if (ready)
    {
        auto& manager = getServiceProvider().get<ICoreWindowManager>();
        auto& status = manager.as<IBrowserWindowStatus&>();
        auto& window = manager.getActiveWindow();
        EXPECT_EQ(&window, &manager.getActiveWindow());
        EXPECT_EQ(status.getBrowserState(), BrowserWindowState::Ready);
        EXPECT_FALSE(manager.createWindow());
        EXPECT_TRUE(window.isVisible());
        window.setSize(320, 240);
        EXPECT_EQ(window.getSize(), (eastl::pair<unsigned, unsigned>{320, 240}));
        EXPECT_EQ(window.getClientSize(), (eastl::pair<unsigned, unsigned>{316, 236}));
        window.setVisible(false);
        EXPECT_FALSE(window.isVisible());
        EXPECT_EQ(window.getSize().first, 320u);
        window.setVisible(true);
        EXPECT_TRUE(window.isVisible());
        // clang-format off
        MAIN_THREAD_EM_ASM({ document.getElementById('parent').style.display = 'none'; });
        // clang-format on
        EXPECT_FALSE(window.isVisible());
        // clang-format off
        MAIN_THREAD_EM_ASM({ document.getElementById('parent').style.display = ''; });
        // clang-format on
        window.setName("Browser test");
        // clang-format off
        EXPECT_EQ(MAIN_THREAD_EM_ASM_INT({ return document.getElementById('nau-window').getAttribute('aria-label') === 'Browser test'; }), 1);
        // clang-format on
        window.setPosition(20, 30);
        EXPECT_EQ(window.getPosition().first, 0u);
        // clang-format off
        EXPECT_EQ(MAIN_THREAD_EM_ASM_INT({ return document.title === 'Nau platform fixture'; }), 1);
        // clang-format on
        // clang-format off
        MAIN_THREAD_EM_ASM({ document.getElementById('nau-window').style.maxWidth = '200px'; });
        // clang-format on
        window.setSize(400, 240);
        EXPECT_EQ(window.getSize().first, 224u);
        // clang-format off
        MAIN_THREAD_EM_ASM({ document.getElementById('nau-window').style.width = '150px'; });
        // clang-format on
        EXPECT_EQ(window.getSize().first, 174u);
        auto executor = manager.getExecutor();
        const auto callerThread = std::this_thread::get_id();
        auto threadCheck = async::run([&]
        {
            return std::this_thread::get_id() != callerThread;
        }, executor);
        EXPECT_TRUE(*async::waitResult(std::move(threadCheck)));
        auto pumpCheck = async::run([&]
        {
            const auto start = std::chrono::steady_clock::now();
            EXPECT_TRUE(manager.pumpMessageQueue(false, 0ms));
            EXPECT_LT(std::chrono::steady_clock::now() - start, 500ms);
            std::atomic<bool> producing = true;
            std::atomic<int> processed = 0;
            for (int i = 0; i < 64; ++i)
            {
                executor->execute([](void* p, void*) noexcept
                {
                    ++*static_cast<std::atomic<int>*>(p);
                    std::this_thread::sleep_for(1ms);
                }, &processed);
            }
            std::thread producer([&]
            {
                while (producing)
                {
                    executor->execute([](void* p, void*) noexcept
                    {
                        ++*static_cast<std::atomic<int>*>(p);
                    }, &processed);
                    std::this_thread::sleep_for(1ms);
                }
            });
            EXPECT_TRUE(manager.pumpMessageQueue(true, 5ms));
            EXPECT_LT(processed, 64);
            EXPECT_LT(std::chrono::steady_clock::now() - start, 500ms);
            producing = false;
            producer.join();
            // Drain the remaining callbacks before their captured counter goes away.
            EXPECT_TRUE(manager.pumpMessageQueue(false, 1000ms));
            EXPECT_GT(processed, 0);
        }, executor);
        EXPECT_TRUE(async::waitResult(std::move(pumpCheck)));
        std::atomic<int> calls = 0;
        executor->execute([](void* value, void*) noexcept
        {
            ++*static_cast<std::atomic<int>*>(value);
        }, &calls);
        const auto deadline = std::chrono::steady_clock::now() + 5s;
        while (calls != 1 && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        EXPECT_EQ(calls, 1);
        // The runner changes document visibility only after this worker is ready.
        // clang-format off
        MAIN_THREAD_EM_ASM({ Module['visibilityReady'] = true; });
        // clang-format on
        const auto hiddenDeadline = std::chrono::steady_clock::now() + 10s;
        while (status.isDocumentVisible() && std::chrono::steady_clock::now() < hiddenDeadline)
            std::this_thread::sleep_for(1ms);
        EXPECT_FALSE(status.isDocumentVisible());
        EXPECT_TRUE(window.isVisible());
        // clang-format off
        MAIN_THREAD_EM_ASM({ Module['visibilitySeen'] = true; });
        // clang-format on
        const auto visibleDeadline = std::chrono::steady_clock::now() + 10s;
        while (!status.isDocumentVisible() && std::chrono::steady_clock::now() < visibleDeadline)
            std::this_thread::sleep_for(1ms);
        EXPECT_TRUE(status.isDocumentVisible());
        // clang-format off
        if (MAIN_THREAD_EM_ASM_INT({ return Module['scenario'] === 'hostloss'; }))
        // clang-format on
        {
            // clang-format off
            MAIN_THREAD_EM_ASM({ document.getElementById('nau-window').remove(); });
            // clang-format on
            const auto detachDeadline = std::chrono::steady_clock::now() + 5s;
            while (status.getBrowserState() == BrowserWindowState::Ready && std::chrono::steady_clock::now() < detachDeadline)
                std::this_thread::sleep_for(1ms);
            EXPECT_EQ(status.getBrowserState(), BrowserWindowState::Failed);
        }
        for (int i = 0; i < 100; ++i)
            executor->execute([](void* value, void*) noexcept
            {
                ++*static_cast<std::atomic<int>*>(value);
            }, &calls);
        auto queuedTask = async::run([]
        {
            return 42;
        }, executor);
        std::atomic<bool> mutate = true;
        std::thread mutator([&]
        {
            while (mutate)
            {
                window.setName("Concurrent operation");
                std::this_thread::sleep_for(1ms);
            }
        });
        EXPECT_TRUE(async::waitResult(service->as<IAsyncDisposable&>().disposeAsync()));
        mutate = false;
        mutator.join();
        EXPECT_EQ(*async::waitResult(std::move(queuedTask)), 42);
        EXPECT_EQ(calls, 101);
        EXPECT_EQ(status.getBrowserState(), BrowserWindowState::Closed);
        window.setVisible(false);
        EXPECT_TRUE(async::waitResult(service->as<IAsyncDisposable&>().disposeAsync()));
        auto lateTask = async::run([]
        {
            return 7;
        }, executor);
        EXPECT_EQ(*async::waitResult(std::move(lateTask)), 7);
        executor.reset();
    }
    else
    {
        EXPECT_FALSE(getServiceProvider().has<ICoreWindowManager>());
        EXPECT_TRUE(async::waitResult(service->as<IAsyncDisposable&>().disposeAsync()));
    }
    service.reset();
    setDefaultServiceProvider(nullptr);
    modules.reset();
    auto shutdown = runtime->shutdown();
    const auto deadline = std::chrono::steady_clock::now() + 10s;
    bool pending = shutdown();
    while (pending && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(1ms);
        pending = shutdown();
    }
    EXPECT_FALSE(pending);
    logSubscription.release();
    diag::setLogger(nullptr);
    // clang-format off
    EXPECT_NE(MAIN_THREAD_EM_ASM_INT({ return Module['scenario'] === 'fixture-failure'; }), 1);
    // clang-format on
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
