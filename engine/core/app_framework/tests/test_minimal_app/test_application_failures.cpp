// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include <thread>

#include "nau/app/application_lifecycle.h"
#include "nau/app/application_services.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/diag/device_error.h"
#include "nau/diag/logging.h"
#include "nau/module/module_manager.h"
#include "nau/runtime/async_disposable.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_object_registry.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

namespace nau::test
{
    class CreationResource final : public IRttiObject,
                                   public IAsyncDisposable
    {
        NAU_RTTI_CLASS(CreationResource, IRttiObject, IAsyncDisposable)
    public:
        CreationResource(int& disposed, int& destroyed) :
            m_disposed(disposed),
            m_destroyed(destroyed)
        {
        }
        ~CreationResource()
        {
            ++m_destroyed;
        }
        async::Task<> disposeAsync() override
        {
            ++m_disposed;
            return async::Task<>::makeRejected(NauMakeError("cleanup diagnostic"));
        }

    private:
        int& m_disposed;
        int& m_destroyed;
    };

    class FailingCreation final : public ApplicationInitDelegate
    {
    public:
        explicit FailingCreation(bool configuration) :
            failConfiguration(configuration)
        {
        }
        Result<> configureApplication() override
        {
            getServiceProvider().addService(eastl::make_unique<CreationResource>(disposed, destroyed));
            return failConfiguration ? Result<>{NauMakeError("configuration diagnostic")} : Result<>{};
        }
        Result<> initializeApplication() override
        {
            ++setupCalls;
            NauCheckResult(loadModulesList(NAU_MODULES_LIST))
            return NauMakeError("module setup diagnostic");
        }
        bool failConfiguration;
        int setupCalls = 0;
        int disposed = 0;
        int destroyed = 0;
    };

    void expectCreationResourcesReleased()
    {
        EXPECT_FALSE(applicationExists());
        EXPECT_FALSE(hasServiceProvider());
        EXPECT_FALSE(hasModuleManager());
        EXPECT_FALSE(RuntimeObjectRegistry::hasInstance());
        EXPECT_FALSE(diag::hasLogger());
        EXPECT_EQ(diag::getDeviceError(), nullptr);
        EXPECT_EQ(async::Executor::getDefault(), nullptr);
    }

    TEST(ApplicationFailures, ConfigurationFailureUnwindsResources)
    {
        FailingCreation delegate(true);
        auto result = createApplicationChecked(delegate);
        ASSERT_FALSE(result);
        const auto message = result.getError()->getMessage();
        EXPECT_NE(message.find("configuration diagnostic"), eastl::string::npos);
        EXPECT_NE(message.find("cleanup diagnostic"), eastl::string::npos);
        EXPECT_EQ(delegate.setupCalls, 0);
        EXPECT_EQ(delegate.disposed, 1);
        EXPECT_EQ(delegate.destroyed, 1);
        expectCreationResourcesReleased();
    }

    TEST(ApplicationFailures, ModuleSetupFailureUnwindsResources)
    {
        FailingCreation delegate(false);
        auto result = createApplicationChecked(delegate);
        ASSERT_FALSE(result);
        EXPECT_NE(result.getError()->getMessage().find("module setup diagnostic"), eastl::string::npos);
        EXPECT_EQ(delegate.setupCalls, 1);
        EXPECT_EQ(delegate.disposed, 1);
        EXPECT_EQ(delegate.destroyed, 1);
        expectCreationResourcesReleased();
    }

    TEST(ApplicationFailures, LegacyCreationReturnsNullAfterCleanup)
    {
        FailingCreation delegate(false);
        EXPECT_EQ(createApplication(delegate), nullptr);
        EXPECT_EQ(delegate.destroyed, 1);
        expectCreationResourcesReleased();
    }

    struct StartupState
    {
        async::TaskSource<>* gate = nullptr;
        bool fail = false;
        bool delayPreInit = false;
        int initialized = 0;
        int updates = 0;
        int destroyed = 0;
    };

    class StartupService final : public IServiceInitialization,
                                 public IGamePreUpdate
    {
        NAU_RTTI_CLASS(StartupService, IServiceInitialization, IGamePreUpdate)
    public:
        explicit StartupService(StartupState& state) :
            m_state(state)
        {
        }
        ~StartupService()
        {
            ++m_state.destroyed;
        }
        async::Task<> preInitService() override
        {
            return m_state.delayPreInit ? m_state.gate->getTask() : async::makeResolvedTask();
        }
        async::Task<> initService() override
        {
            ++m_state.initialized;
            if (m_state.gate && !m_state.delayPreInit)
                return m_state.gate->getTask();
            return m_state.fail ? async::Task<>::makeRejected(NauMakeError("startup diagnostic")) : async::makeResolvedTask();
        }
        void gamePreUpdate(std::chrono::milliseconds) override
        {
            ++m_state.updates;
        }

    private:
        StartupState& m_state;
    };

    class StartupDelegate final : public ApplicationInitDelegate
    {
    public:
        explicit StartupDelegate(StartupState& state) :
            m_state(state)
        {
        }
        Result<> configureApplication() override
        {
            return {};
        }
        Result<> initializeApplication() override
        {
            getServiceProvider().addService(eastl::make_unique<StartupService>(m_state));
            return {};
        }

    private:
        StartupState& m_state;
    };

    struct PendingCleanupState
    {
        async::TaskSource<> release;
        int disposed = 0;
        int asyncDisposed = 0;
        int destroyed = 0;
    };

    class RegisteredCleanupResource final : public IDisposable,
                                            public IAsyncDisposable
    {
        NAU_RTTI_CLASS(RegisteredCleanupResource, IDisposable, IAsyncDisposable)
    public:
        explicit RegisteredCleanupResource(PendingCleanupState& state) :
            m_state(state),
            m_registration(*this)
        {
        }
        ~RegisteredCleanupResource()
        {
            ++m_state.destroyed;
        }
        void dispose() override
        {
            ++m_state.disposed;
        }
        async::Task<> disposeAsync() override
        {
            ++m_state.asyncDisposed;
            return m_state.release.getTask();
        }

    private:
        PendingCleanupState& m_state;
        RuntimeObjectRegistration m_registration;
    };

    bool drainApplication(Application& app)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (app.step())
        {
            if (std::chrono::steady_clock::now() >= deadline)
                return false;
            std::this_thread::yield();
        }
        return true;
    }

    TEST(ApplicationFailures, LegacyStartupFailureNeverEntersUpdates)
    {
        StartupState state;
        state.fail = true;
        StartupDelegate delegate(state);
        auto app = createApplication(delegate);
        ASSERT_TRUE(app);
        app->startupOnCurrentThread();
        EXPECT_TRUE(app->as<ApplicationLifecycle&>().getLifecycleError());
        EXPECT_TRUE(drainApplication(*app));
        EXPECT_EQ(state.updates, 0);
        EXPECT_EQ(state.destroyed, 1);
        app.reset();
        expectCreationResourcesReleased();
    }

    TEST(ApplicationFailures, DeferredStartupFailureIsPumpable)
    {
        StartupState state;
        async::TaskSource<> gate;
        state.gate = &gate;
        StartupDelegate delegate(state);
        auto app = createApplication(delegate);
        ASSERT_TRUE(app);
        auto& lifecycle = app->as<ApplicationLifecycle&>();
        lifecycle.beginStartup();
        lifecycle.pollLifecycle();
        EXPECT_EQ(lifecycle.getPhase(), ApplicationPhase::Initializing);
        EXPECT_EQ(state.initialized, 1);
        EXPECT_EQ(state.updates, 0);
        gate.reject(NauMakeError("deferred startup diagnostic"));
        EXPECT_TRUE(drainApplication(*app));
        ASSERT_TRUE(lifecycle.getLifecycleError());
        EXPECT_NE(lifecycle.getLifecycleError()->getMessage().find("deferred startup diagnostic"), eastl::string::npos);
        EXPECT_EQ(state.updates, 0);
        EXPECT_EQ(state.destroyed, 1);
        app.reset();
        expectCreationResourcesReleased();
    }

    TEST(ApplicationFailures, StopDuringPreInitializationWaitsThenSkipsInitialization)
    {
        StartupState state;
        async::TaskSource<> gate;
        state.gate = &gate;
        state.delayPreInit = true;
        StartupDelegate delegate(state);
        auto app = createApplication(delegate);
        ASSERT_TRUE(app);
        auto& lifecycle = app->as<ApplicationLifecycle&>();
        lifecycle.beginStartup();
        app->stop();
        app->stop();
        EXPECT_TRUE(lifecycle.pollLifecycle());
        EXPECT_EQ(state.destroyed, 0);
        EXPECT_EQ(state.initialized, 0);
        gate.resolve();
        EXPECT_TRUE(drainApplication(*app));
        EXPECT_FALSE(lifecycle.getLifecycleError());
        EXPECT_EQ(state.initialized, 0);
        EXPECT_EQ(state.updates, 0);
        EXPECT_EQ(state.destroyed, 1);
        app->stop();
        EXPECT_FALSE(app->step());
        app.reset();
        expectCreationResourcesReleased();
    }

    TEST(ApplicationFailures, PendingCleanupRetainsObjectsWithoutUpdatesOrDuplicateDisposal)
    {
        StartupState state;
        PendingCleanupState cleanup;
        StartupDelegate delegate(state);
        auto app = createApplication(delegate);
        ASSERT_TRUE(app);
        getServiceProvider().addService(eastl::make_unique<RegisteredCleanupResource>(cleanup));
        app->startupOnCurrentThread();
        EXPECT_TRUE(app->step());
        const int updates = state.updates;
        app->stop();
        for (int i = 0; i < 10; ++i)
            EXPECT_TRUE(app->step());
        EXPECT_EQ(cleanup.asyncDisposed, 1);
        EXPECT_EQ(cleanup.disposed, 1);
        EXPECT_EQ(cleanup.destroyed, 0);
        EXPECT_EQ(state.updates, updates);
        cleanup.release.resolve();
        EXPECT_TRUE(drainApplication(*app));
        EXPECT_EQ(cleanup.disposed, 1);
        EXPECT_EQ(cleanup.asyncDisposed, 1);
        EXPECT_EQ(cleanup.destroyed, 1);
        EXPECT_EQ(state.updates, updates);
        app.reset();
        expectCreationResourcesReleased();
    }
}  // namespace nau::test
