// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <vector>

#include "nau/async/task.h"
#include "nau/service/internal/service_provider_initialization.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

namespace nau::test
{
    class TestServiceInitProxy : public testing::Test
    {
    protected:
        /**
         */
        class MyService1 : public IServiceInitialization,
                           public IServiceShutdown
        {
            NAU_RTTI_CLASS(MyService1, IServiceInitialization, IServiceShutdown)

        public:
            void setPreInitialized()
            {
                m_isPreInitialized = true;
            }

            void setInitialized()
            {
                m_isInitialized = true;
            }

            void setShutdown()
            {
                m_isShutdown = true;
            }

            bool isPreInitialized() const
            {
                return m_isPreInitialized;
            }

            bool isInitialized() const
            {
                return m_isInitialized;
            }

            bool isShutdown() const
            {
                return m_isShutdown;
            }

            bool preInitServiceCalled() const
            {
                return m_preInitServiceCalled;
            }

            bool initServiceCalled() const
            {
                return m_initServiceCalled;
            }

            bool shutdownServiceCalled() const
            {
                return m_shutdownServiceCalled;
            }

        private:
            async::Task<> preInitService() override
            {
                m_preInitServiceCalled = true;
                co_return;
            }

            async::Task<> initService() override
            {
                m_initServiceCalled = true;
                co_return;
            }

            async::Task<> shutdownService() override
            {
                m_shutdownServiceCalled = true;
                co_return;
            }

            bool m_isPreInitialized = false;
            bool m_isInitialized = false;
            bool m_isShutdown = false;
            bool m_preInitServiceCalled = false;
            bool m_initServiceCalled = false;
            bool m_shutdownServiceCalled = false;
        };

        class MyService2 : public IServiceInitialization,
                           public IServiceShutdown
        {
            NAU_RTTI_CLASS(MyService2, IServiceInitialization, IServiceShutdown)

        public:
            bool isPreInitOk() const
            {
                return m_preInitOk;
            }

            bool isInitOk() const
            {
                return m_initOk;
            }

            bool isShutdownOk() const
            {
                return m_shutdownOk;
            }

            bool shutdownServiceCalled() const
            {
                return m_shutdownServiceCalled;
            }

        private:
            async::Task<> preInitService() override
            {
                m_preInitOk = getServiceProvider().get<MyService1>().isPreInitialized();
                co_return;
            }

            async::Task<> initService() override
            {
                m_initOk = getServiceProvider().get<MyService1>().isInitialized();
                co_return;
            }

            std::vector<const rtti::TypeInfo*> getServiceDependencies() const override
            {
                return {&rtti::getTypeInfo<MyService1>()};
            }

            async::Task<> shutdownService() override
            {
                m_shutdownServiceCalled = true;
                m_shutdownOk = !getServiceProvider().get<MyService1>().isShutdown();
                co_return;
            }

            bool m_preInitOk = false;
            bool m_initOk = false;
            bool m_shutdownOk = false;
            bool m_shutdownServiceCalled = false;
        };

        /**
         */
        class MyServiceInitProxy : public IServiceInitialization,
                                   public IServiceShutdown
        {
            NAU_RTTI_CLASS(MyServiceInitProxy, IServiceInitialization, IServiceShutdown)
        public:
            MyServiceInitProxy(MyService1& service1) :
                m_service1(service1)
            {
            }

        private:
            async::Task<> preInitService() override
            {
                m_service1.setPreInitialized();
                co_return;
            }

            async::Task<> initService() override
            {
                m_service1.setInitialized();
                co_return;
            }

            async::Task<> shutdownService() override
            {
                m_service1.setShutdown();
                co_return;
            }

            MyService1& m_service1;
        };

        /**
         */
        class MyServiceInitProxy2 : public IServiceInitialization
        {
            NAU_RTTI_CLASS(MyServiceInitProxy2, IServiceInitialization)
        };

    private:
        void SetUp() override
        {
            setDefaultServiceProvider(createServiceProvider());
        }

        void TearDown() override
        {
            setDefaultServiceProvider(nullptr);
        }
    };

    /**
        Test: Service initialization through proxy.

            * register MyService1, MyService2.
            * for MyService1 setting initialization proxy: expecting that servicePreInit, serviceInit should be called for MyServiceInitProxy, but not MyService1
            * checks that MyService1::preInitService, MyService1::initService was not called
            * checks that MyServiceInitProxy::preInitService, MyServiceInitProxy::initService are called (and MyService1 corresponding flags are set)
            * checks that service inter dependencies are satisfied: MyService2 initialized after MyService1 (actually MyServiceInitProxy)

     */
    TEST_F(TestServiceInitProxy, SetProxy_Init)
    {
        std::unique_ptr<MyService1> service1 = std::make_unique<MyService1>();
        std::unique_ptr<IServiceInitialization> proxy = std::make_unique<MyServiceInitProxy>(*service1);

        auto& serviceProvider = getServiceProvider();
        auto& serviceProviderInit = serviceProvider.as<core_detail::IServiceProviderInitialization&>();

        serviceProviderInit.setInitializationProxy(service1->as<const IServiceInitialization&>(), proxy.get());
        serviceProvider.addService<MyService2>();
        serviceProvider.addService(std::move(service1));

        async::waitResult(serviceProviderInit.preInitServices()).ignore();
        async::waitResult(serviceProviderInit.initServices()).ignore();

        ASSERT_FALSE(serviceProvider.get<MyService1>().preInitServiceCalled());
        ASSERT_FALSE(serviceProvider.get<MyService1>().initServiceCalled());

        ASSERT_TRUE(serviceProvider.get<MyService1>().isPreInitialized());
        ASSERT_TRUE(serviceProvider.get<MyService1>().isInitialized());

        ASSERT_TRUE(serviceProvider.get<MyService2>().isPreInitOk());
        ASSERT_TRUE(serviceProvider.get<MyService2>().isInitOk());
    }

    /**
        Test: Service shutdown through proxy.

        * MyService1::serviceShutdown() must not be called, instead MyServiceInitProxy::serviceShutdown() must be called
        * MyService2 has proxy MyServiceInitProxy2, but proxy does not expose IServiceShutdown, so expected to be called MyService2::serviceShutdown()
        * checks invocation ordering (MyService1 must shutting down prior MyService1, because of dependencies)
     */
    TEST_F(TestServiceInitProxy, SetProxy_Shutdown)
    {
        auto& serviceProvider = getServiceProvider();
        auto& serviceProviderInit = serviceProvider.as<core_detail::IServiceProviderInitialization&>();

        std::unique_ptr<MyService1> service1 = std::make_unique<MyService1>();
        std::unique_ptr<MyService2> service2 = std::make_unique<MyService2>();

        std::unique_ptr<IServiceInitialization> proxy1 = std::make_unique<MyServiceInitProxy>(*service1);
        std::unique_ptr<IServiceInitialization> proxy2 = std::make_unique<MyServiceInitProxy2>();

        serviceProviderInit.setInitializationProxy(service1->as<const IServiceInitialization&>(), proxy1.get());
        serviceProviderInit.setInitializationProxy(service2->as<const IServiceInitialization&>(), proxy2.get());
        serviceProvider.addService(std::move(service1));
        serviceProvider.addService(std::move(service2));

        async::waitResult(serviceProviderInit.preInitServices()).ignore();
        async::waitResult(serviceProviderInit.initServices()).ignore();
        async::waitResult(serviceProviderInit.shutdownServices()).ignore();

        ASSERT_FALSE(serviceProvider.get<MyService1>().shutdownServiceCalled());
        ASSERT_TRUE(serviceProvider.get<MyService1>().isShutdown());

        ASSERT_TRUE(serviceProvider.get<MyService2>().shutdownServiceCalled());
        ASSERT_TRUE(serviceProvider.get<MyService2>().isShutdownOk());
    }

}  // namespace nau::test
