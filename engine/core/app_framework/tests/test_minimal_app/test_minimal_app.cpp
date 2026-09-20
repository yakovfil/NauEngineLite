// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <thread>

#include "nau/app/application.h"
#include "nau/app/application_services.h"
#include "nau/app/background_work_service.h"
#include "nau/app/core_window_manager.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/io/virtual_file_system.h"
#include "nau/module/module_manager.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

namespace nau::test
{
    struct LifecycleState
    {
        int preUpdates = 0;
        int postUpdates = 0;
        bool initialized = false;
        bool shutdown = false;
        bool destroyed = false;
    };

    class LifecycleObserver final : public IServiceInitialization,
                                    public IServiceShutdown,
                                    public IGamePreUpdate,
                                    public IGamePostUpdate
    {
        NAU_RTTI_CLASS(nau::test::LifecycleObserver, IServiceInitialization, IServiceShutdown, IGamePreUpdate, IGamePostUpdate)

    public:
        explicit LifecycleObserver(LifecycleState& state) :
            m_state(state)
        {
        }

        ~LifecycleObserver()
        {
            m_state.destroyed = true;
        }

        async::Task<> initService() override
        {
            m_state.initialized = true;
            return async::makeResolvedTask();
        }

        async::Task<> shutdownService() override
        {
            m_state.shutdown = true;
            return async::makeResolvedTask();
        }

        void gamePreUpdate(std::chrono::milliseconds) override
        {
            ++m_state.preUpdates;
        }

        void gamePostUpdate(std::chrono::milliseconds) override
        {
            EXPECT_EQ(m_state.preUpdates, m_state.postUpdates + 1);
            ++m_state.postUpdates;
        }

    private:
        LifecycleState& m_state;
    };

    TEST(MinimalRuntime, RealPlatformLifecycle)
    {
        LifecycleState state;
        auto app = createApplication([&state]() -> Result<>
        {
            NauCheckResult(loadModulesList(NAU_MODULES_LIST))
            getServiceProvider().addService(createPlatformWindowService());
            getServiceProvider().addService(eastl::make_unique<LifecycleObserver>(state));
            return ResultSuccess;
        });
        ASSERT_NE(app, nullptr);
        app->startupOnCurrentThread();
        EXPECT_TRUE(app->isMainThread());
        EXPECT_TRUE(app->hasExecutor());
        EXPECT_TRUE(state.initialized);
        EXPECT_TRUE(getServiceProvider().has<ICoreWindowManager>());
        EXPECT_TRUE(getServiceProvider().has<BackgroundWorkService>());
        EXPECT_TRUE(getServiceProvider().has<io::IVirtualFileSystem>());

        for (int i = 0; i < 5; ++i)
        {
            EXPECT_TRUE(app->step());
        }
        EXPECT_EQ(state.preUpdates, 5);
        EXPECT_EQ(state.postUpdates, 5);

        app->stop();
        EXPECT_TRUE(app->isClosing());
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
        bool running = true;
        while (running && std::chrono::steady_clock::now() < deadline)
        {
            running = app->step();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        EXPECT_FALSE(running);
        EXPECT_TRUE(state.shutdown);
        EXPECT_TRUE(state.destroyed);
        EXPECT_FALSE(app->step());
        app.reset();
        EXPECT_FALSE(applicationExists());
    }
}  // namespace nau::test
