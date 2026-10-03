// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <vector>

#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/internal/runtime_state.h"
#include "nau/service/internal/service_provider_initialization.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

using namespace testing;

namespace nau::test
{
    class TestServiceDependencies : public testing::Test
    {
    protected:
        ServiceProvider::Ptr m_serviceProvider = createServiceProvider();
    };

    namespace
    {
        struct ITestInterface1 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface1, IRttiObject)
        };

        struct ITestInterface1_2 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface1_2, IRttiObject)
        };

        struct ITestInterface2 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface2, IRttiObject)
        };

        struct ITestInterface3 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface3, IRttiObject)
        };

        struct ITestInterface4 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface4, IRttiObject)
        };

        struct ServiceInitData : IServiceInitialization,
                                 IServiceShutdown
        {
            NAU_INTERFACE(ServiceInitData, IServiceInitialization, IServiceShutdown)

            ServiceInitData(ServiceProvider& inServiceProvider) :
                serviceProvider(inServiceProvider)
            {
            }

            virtual bool isOperable() const = 0;

            ServiceProvider& serviceProvider;
            bool isPreInitialized = false;
            bool isInitialized = false;
            bool isInitializedSuccess = false;

            bool isShutDown = false;
            bool isShutDownSuccess = false;
        };

        template <typename Itf, typename... Dependency>
        class ServiceWithInit : public Itf,
                                public ServiceInitData
        {
            NAU_RTTI_CLASS(ServiceWithInit<Itf>, Itf, ServiceInitData)

        public:
            ServiceWithInit(ServiceProvider& inServiceProvider) :
                ServiceInitData{inServiceProvider}
            {
            }

            async::Task<> preInitService() override
            {
                if constexpr (sizeof...(Dependency) == 0)
                {
                    this->isPreInitialized = true;
                    co_return;
                }

                auto checkDepsIsReady = [this]<typename U>(TypeList<U>) -> bool
                {
                    U& deps = this->serviceProvider.template get<U>();
                    const bool depsIsReady = deps.template as<const ServiceInitData&>().isPreInitialized;
                    return depsIsReady;
                };

                auto checkAllDepsAreReady = [&]<typename... U>(TypeList<U...>)
                {
                    return (checkDepsIsReady(TypeList<U>{}) && ...);
                };

                this->isPreInitialized = checkAllDepsAreReady(TypeList<Dependency...>{});

                co_return;
            }

            async::Task<> initService() override
            {
                this->isInitialized = true;
                this->isInitializedSuccess = this->isOperable();

                return async::makeResolvedTask();
            }

            async::Task<> shutdownService() override
            {
                this->isShutDownSuccess = this->isOperable();
                this->isShutDown = true;

                return async::makeResolvedTask();
            }

            bool isOperable() const override
            {
                if constexpr (sizeof...(Dependency) == 0)
                {
                    return this->isPreInitialized && this->isInitialized && !this->isShutDown;
                }
                else
                {
                    auto checkDepsIsOperable = [this]<typename U>(TypeList<U>) -> bool
                    {
                        U& deps = this->serviceProvider.template get<U>();
                        const ServiceInitData& initData = deps.template as<const ServiceInitData&>();

                        return initData.isOperable();
                    };

                    auto checkAllDepsAreOperable = [&]<typename... U>(TypeList<U...>)
                    {
                        return (checkDepsIsOperable(TypeList<U>{}) && ...);
                    };

                    return this->isPreInitialized && this->isInitialized && !this->isShutDown && checkAllDepsAreOperable(TypeList<Dependency...>{});
                }
            }

            std::vector<const rtti::TypeInfo*> getServiceDependencies() const override
            {
                return std::vector<const rtti::TypeInfo*>{
                    &rtti::getTypeInfo<Dependency>()...};
            }
        };

        /**
         */
        class ServiceShutdownOnly final : public IServiceShutdown
        {
            NAU_RTTI_CLASS(ServiceShutdownOnly, IServiceShutdown)

        public:
            bool isShutDown() const
            {
                return m_isShutDown;
            }

        private:
            async::Task<> shutdownService() override
            {
                m_isShutDown = true;
                return async::makeResolvedTask();
            }

            bool m_isShutDown = false;
        };

        using Service1 = ServiceWithInit<ITestInterface1>;
        using Service2 = ServiceWithInit<ITestInterface2, ITestInterface1>;
        using Service3 = ServiceWithInit<ITestInterface3, ITestInterface2>;
        using Service4 = ServiceWithInit<ITestInterface4, ITestInterface1, ITestInterface3>;
    }  // namespace

    /**
        Test:
            the order of initialization of services takes into account the dependencies between them
     */
    TEST_F(TestServiceDependencies, Initialization)
    {
        m_serviceProvider->addService(std::make_unique<Service3>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<Service4>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<Service2>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<Service1>(*m_serviceProvider));

        const auto isPreInitialized = [this]<typename T>(TypeList<T>)
        {
            const T& instance = m_serviceProvider->get<T>();
            return instance.template as<const ServiceInitData&>().isPreInitialized;
        };

        const auto isInitializedOk = [this]<typename T>(TypeList<T>)
        {
            const T& instance = m_serviceProvider->get<T>();
            return instance.template as<const ServiceInitData&>().isInitializedSuccess;
        };

        auto& serviceProviderInit = m_serviceProvider->as<core_detail::IServiceProviderInitialization&>();
        auto resPreInit = async::waitResult(serviceProviderInit.preInitServices());
        ASSERT_TRUE(resPreInit);
        ASSERT_TRUE(isPreInitialized(TypeList<ITestInterface1>{}));
        ASSERT_TRUE(isPreInitialized(TypeList<ITestInterface2>{}));
        ASSERT_TRUE(isPreInitialized(TypeList<ITestInterface3>{}));
        ASSERT_TRUE(isPreInitialized(TypeList<ITestInterface4>{}));

        auto resInit = async::waitResult(serviceProviderInit.initServices());
        ASSERT_TRUE(resInit);

        ASSERT_TRUE(isInitializedOk(TypeList<ITestInterface1>{}));
        ASSERT_TRUE(isInitializedOk(TypeList<ITestInterface2>{}));
        ASSERT_TRUE(isInitializedOk(TypeList<ITestInterface3>{}));
        ASSERT_TRUE(isInitializedOk(TypeList<ITestInterface4>{}));
    }

    /**
        Test:
            the order of services shutdown takes into account the dependencies between them: it must be reverse of the initialization sequence
     */
    TEST_F(TestServiceDependencies, Shutdown)
    {
        m_serviceProvider->addService(std::make_unique<Service1>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<Service3>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<Service4>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<Service2>(*m_serviceProvider));
        m_serviceProvider->addService(std::make_unique<ServiceShutdownOnly>());

        auto& serviceProviderInit = m_serviceProvider->as<core_detail::IServiceProviderInitialization&>();
        auto resPreInit = async::waitResult(serviceProviderInit.preInitServices());
        ASSERT_TRUE(resPreInit);

        auto resInit = async::waitResult(serviceProviderInit.initServices());
        ASSERT_TRUE(resInit);

        auto resShutdown = async::waitResult(serviceProviderInit.shutdownServices());
        ASSERT_TRUE(resShutdown);

        const auto isShutDownOk = [this]<typename T>(TypeList<T>)
        {
            const T& instance = m_serviceProvider->get<T>();
            return instance.template as<const ServiceInitData&>().isShutDownSuccess;
        };

        EXPECT_TRUE(isShutDownOk(TypeList<ITestInterface1>{}));
        EXPECT_TRUE(isShutDownOk(TypeList<ITestInterface2>{}));
        EXPECT_TRUE(isShutDownOk(TypeList<ITestInterface3>{}));
        EXPECT_TRUE(isShutDownOk(TypeList<ITestInterface4>{}));
        EXPECT_TRUE(m_serviceProvider->get<ServiceShutdownOnly>().isShutDown());
    }

}  // namespace nau::test
