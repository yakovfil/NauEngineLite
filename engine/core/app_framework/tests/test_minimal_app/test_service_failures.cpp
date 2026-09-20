// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include "nau/runtime/async_disposable.h"
#include "nau/service/internal/service_provider_initialization.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

namespace nau::test
{
    struct ServiceFailureState
    {
        bool failPreInit = false;
        bool failInit = false;
        bool failShutdown = false;
        bool failDispose = false;
        async::TaskSource<>* initGate = nullptr;
        async::TaskSource<>* shutdownGate = nullptr;
        async::TaskSource<>* disposeGate = nullptr;
        int initialized = 0;
        int shutdown = 0;
        int disposed = 0;
        int destroyed = 0;
        eastl::vector<int>* order = nullptr;
    };

    template <int Id, typename Dependency = void>
    class FailureService final : public IServiceInitialization,
                                 public IServiceShutdown,
                                 public IAsyncDisposable
    {
        NAU_RTTI_CLASS(FailureService<Id>, IServiceInitialization, IServiceShutdown, IAsyncDisposable)
    public:
        explicit FailureService(ServiceFailureState& state) :
            m_state(state)
        {
        }
        ~FailureService()
        {
            ++m_state.destroyed;
        }

        async::Task<> preInitService() override
        {
            return m_state.failPreInit ? async::Task<>::makeRejected(NauMakeError("pre-init failure")) : async::makeResolvedTask();
        }

        async::Task<> initService() override
        {
            ++m_state.initialized;
            if (m_state.initGate)
            {
                return m_state.initGate->getTask();
            }
            return m_state.failInit ? async::Task<>::makeRejected(NauMakeError("init failure")) : async::makeResolvedTask();
        }

        async::Task<> shutdownService() override
        {
            ++m_state.shutdown;
            if (m_state.order)
                m_state.order->push_back(Id);
            if (m_state.shutdownGate)
                return m_state.shutdownGate->getTask();
            return m_state.failShutdown ? async::Task<>::makeRejected(NauMakeError("shutdown failure")) : async::makeResolvedTask();
        }

        async::Task<> disposeAsync() override
        {
            ++m_state.disposed;
            if (m_state.disposeGate)
                return m_state.disposeGate->getTask();
            return m_state.failDispose ? async::Task<>::makeRejected(NauMakeError("dispose failure")) : async::makeResolvedTask();
        }

        eastl::vector<const rtti::TypeInfo*> getServiceDependencies() const override
        {
            if constexpr (!std::is_void_v<Dependency>)
                return {&rtti::getTypeInfo<Dependency>()};
            return {};
        }

    private:
        ServiceFailureState& m_state;
    };

    TEST(ServiceFailures, ImmediateFailureSettlesStartedTasksAndSkipsDependents)
    {
        ServiceFailureState failed, pending, dependent;
        failed.failInit = true;
        async::TaskSource<> gate;
        pending.initGate = &gate;
        auto provider = createServiceProvider();
        provider->addService(eastl::make_unique<FailureService<1>>(failed));
        provider->addService(eastl::make_unique<FailureService<2>>(pending));
        provider->addService(eastl::make_unique<FailureService<3, FailureService<1>>>(dependent));
        auto& lifecycle = provider->as<core_detail::IServiceProviderInitialization&>();
        auto preInit = lifecycle.preInitServices();
        ASSERT_TRUE(preInit.isReady());
        auto init = lifecycle.initServices();
        EXPECT_FALSE(init.isReady());
        EXPECT_EQ(pending.initialized, 1);
        EXPECT_EQ(dependent.initialized, 0);
        gate.resolve();
        ASSERT_TRUE(init.isReady());
        EXPECT_TRUE(init.isRejected());
        auto shutdown = lifecycle.shutdownServices();
        ASSERT_TRUE(shutdown.isReady());
        EXPECT_EQ(failed.shutdown, 0);
        EXPECT_EQ(pending.shutdown, 1);
        EXPECT_EQ(dependent.shutdown, 0);
        EXPECT_EQ(failed.disposed, 1);
        EXPECT_EQ(dependent.disposed, 1);
        provider.reset();
        EXPECT_EQ(failed.destroyed, 1);
        EXPECT_EQ(pending.destroyed, 1);
        EXPECT_EQ(dependent.destroyed, 1);
    }

    TEST(ServiceFailures, DeferredFailureAndImmediateFailureAreBothReported)
    {
        ServiceFailureState first, second;
        async::TaskSource<> gate;
        first.initGate = &gate;
        second.failInit = true;
        auto provider = createServiceProvider();
        provider->addService(eastl::make_unique<FailureService<1>>(first));
        provider->addService(eastl::make_unique<FailureService<2>>(second));
        auto& lifecycle = provider->as<core_detail::IServiceProviderInitialization&>();
        auto init = lifecycle.initServices();
        EXPECT_FALSE(init.isReady());
        EXPECT_EQ(second.initialized, 1);
        ASSERT_TRUE(lifecycle.getLifecycleError());
        EXPECT_NE(lifecycle.getLifecycleError()->getMessage().find("init failure"), eastl::string::npos);
        gate.reject(NauMakeError("deferred failure"));
        ASSERT_TRUE(init.isReady());
        ASSERT_TRUE(init.isRejected());
        const auto message = init.getError()->getMessage();
        EXPECT_NE(message.find("deferred failure"), eastl::string::npos);
        EXPECT_NE(message.find("init failure"), eastl::string::npos);
        auto shutdown = lifecycle.shutdownServices();
        EXPECT_TRUE(shutdown.isReady());
        EXPECT_EQ(first.shutdown + second.shutdown, 0);
        EXPECT_EQ(first.disposed + second.disposed, 2);
    }

    TEST(ServiceFailures, PreInitializationFailurePreventsInitialization)
    {
        ServiceFailureState state;
        state.failPreInit = true;
        auto provider = createServiceProvider();
        provider->addService(eastl::make_unique<FailureService<1>>(state));
        auto& lifecycle = provider->as<core_detail::IServiceProviderInitialization&>();
        auto preInit = lifecycle.preInitServices();
        EXPECT_TRUE(preInit.isRejected());
        auto init = lifecycle.initServices();
        EXPECT_TRUE(init.isRejected());
        EXPECT_EQ(state.initialized, 0);
        auto shutdown = lifecycle.shutdownServices();
        EXPECT_TRUE(shutdown.isReady());
        EXPECT_EQ(state.shutdown, 0);
        EXPECT_EQ(state.disposed, 1);
    }

    TEST(ServiceFailures, CleanupContinuesInDependencyOrderAndRetainsPendingTasks)
    {
        ServiceFailureState first, second, dependent;
        eastl::vector<int> order;
        first.order = second.order = dependent.order = &order;
        first.failShutdown = true;
        dependent.failShutdown = true;
        first.failDispose = true;
        async::TaskSource<> shutdownGate, disposeGate;
        second.shutdownGate = &shutdownGate;
        dependent.disposeGate = &disposeGate;
        auto provider = createServiceProvider();
        provider->addService(eastl::make_unique<FailureService<1>>(first));
        provider->addService(eastl::make_unique<FailureService<2>>(second));
        provider->addService(eastl::make_unique<FailureService<3, FailureService<1>>>(dependent));
        auto& lifecycle = provider->as<core_detail::IServiceProviderInitialization&>();
        auto init = lifecycle.initServices();
        ASSERT_TRUE(init.isReady());
        ASSERT_FALSE(init.isRejected());
        auto shutdown = lifecycle.shutdownServices();
        EXPECT_FALSE(shutdown.isReady());
        ASSERT_EQ(order.size(), 3);
        EXPECT_EQ(order.front(), 3);
        EXPECT_EQ(first.disposed, 0);
        ASSERT_TRUE(lifecycle.getLifecycleError());
        const auto progress = lifecycle.getLifecycleProgress();
        shutdownGate.reject(NauMakeError("deferred shutdown"));
        EXPECT_FALSE(shutdown.isReady());
        EXPECT_NE(lifecycle.getLifecycleProgress(), progress);
        EXPECT_EQ(first.disposed + second.disposed + dependent.disposed, 3);
        EXPECT_EQ(dependent.destroyed, 0);
        disposeGate.reject(NauMakeError("deferred dispose"));
        ASSERT_TRUE(shutdown.isReady());
        ASSERT_TRUE(shutdown.isRejected());
        const auto message = shutdown.getError()->getMessage();
        EXPECT_NE(message.find("shutdown failure"), eastl::string::npos);
        EXPECT_NE(message.find("deferred shutdown"), eastl::string::npos);
        EXPECT_NE(message.find("dispose failure"), eastl::string::npos);
        EXPECT_NE(message.find("deferred dispose"), eastl::string::npos);
        provider.reset();
        EXPECT_EQ(dependent.destroyed, 1);
    }
}  // namespace nau::test
