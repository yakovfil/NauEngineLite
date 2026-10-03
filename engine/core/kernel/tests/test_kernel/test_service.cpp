#include <algorithm>
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
    class TestService : public testing::Test
    {
    protected:
        template <typename T>
        static bool containsClass(const std::vector<IClassDescriptor::Ptr>& classes)
        {
            return std::any_of(classes.begin(), classes.end(), [](const IClassDescriptor::Ptr& classDescriptor)
            {
                return classDescriptor->getClassTypeInfo() == rtti::getTypeInfo<T>();
            });
        }

        ServiceProvider::Ptr m_serviceProvider = createServiceProvider();
    };

    struct InitServiceTestData
    {
        bool asyncMode = false;
        bool lazyCreation = false;
        size_t serviceCount = 100;

        static std::vector<InitServiceTestData> getDefaults()
        {
            return {
                InitServiceTestData{.asyncMode = false, .lazyCreation = false},
                InitServiceTestData{ .asyncMode = true, .lazyCreation = false},
                InitServiceTestData{ .asyncMode = true,  .lazyCreation = true}
            };
        }
    };

    class TestServiceInit : public testing::TestWithParam<InitServiceTestData>
    {
    protected:
        ~TestServiceInit()
        {
            auto shutdown = m_runtime->shutdown();
            while (shutdown())
            {
                std::this_thread::yield();
            }
        }

        void registerAllServices();

        RuntimeState::Ptr m_runtime = RuntimeState::create();
        ServiceProvider::Ptr m_serviceProvider = createServiceProvider();
    };

    namespace
    {
        struct ITestInterface1 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface1, IRttiObject)
        };

        struct ITestInterface1A : ITestInterface1
        {
            NAU_INTERFACE(ITestInterface1A, ITestInterface1)
        };

        struct ITestInterface2 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface2, IRttiObject)
        };

        struct ITestInterface3 : IRttiObject
        {
            NAU_INTERFACE(ITestInterface3, IRttiObject)
        };

        struct ITestRCInterface1 : IRefCounted
        {
            NAU_INTERFACE(ITestRCInterface1, IRefCounted)
        };

        struct ITestRCInterface2 : IRefCounted
        {
            NAU_INTERFACE(ITestRCInterface2, IRefCounted)
        };

        class TestService1 : public ITestInterface1
        {
            NAU_RTTI_CLASS(TestService1, ITestInterface1)

        private:
        };

        class TestService12 : public ITestInterface1,
                              public ITestInterface2
        {
            NAU_RTTI_CLASS(TestService12, ITestInterface1, ITestInterface2)
        };

        class TestService1A2 : public ITestInterface1A,
                               public ITestInterface2
        {
            NAU_RTTI_CLASS(TestService1A2, ITestInterface1A, ITestInterface2)
        };

        class TestService3 : public ITestInterface3
        {
            NAU_RTTI_CLASS(TestService3, ITestInterface3)
        };

        class TestRCService1 : public ITestRCInterface1
        {
            NAU_CLASS_(TestRCService1, ITestRCInterface1)
        };

        class TestRCService12 : public ITestRCInterface1,
                                public ITestRCInterface2
        {
            NAU_CLASS_(TestRCService12, ITestRCInterface1, ITestRCInterface2)
        };

        struct ITestServiceInit : virtual IRttiObject
        {
            NAU_INTERFACE(ITestServiceInit, IRttiObject)

            virtual bool isPreInitialized() const = 0;
            virtual bool isInitialized() const = 0;
        };

        class TestServiceWithInit : public ITestInterface1,
                                    public ITestServiceInit,
                                    public IServiceInitialization
        {
            NAU_RTTI_CLASS(TestRCService1, ITestInterface1, ITestServiceInit, IServiceInitialization)

        public:
            TestServiceWithInit(bool asyncMode) :
                m_isAsyncMode(asyncMode)
            {
            }

        private:
            bool isPreInitialized() const override
            {
                return m_isPreInitialized;
            }

            bool isInitialized() const override
            {
                return m_isInitialized;
            }

            async::Task<> preInitService() override
            {
                if (m_isAsyncMode)
                {
                    co_await async::Executor::getDefault();
                }

                m_isPreInitialized = true;
                co_return;
            }

            async::Task<> initService() override
            {
                if (m_isAsyncMode)
                {
                    co_await async::Executor::getDefault();
                }

                m_isInitialized = true;
                co_return;
            }

            const bool m_isAsyncMode;
            bool m_isPreInitialized = false;
            bool m_isInitialized = false;
        };

    }  // namespace

    void TestServiceInit::registerAllServices()
    {
        const bool useAsync = GetParam().asyncMode;
        const bool lazyCreation = GetParam().lazyCreation;

        for (size_t i = 0, count = GetParam().serviceCount; i < count; ++i)
        {
            if (lazyCreation)
            {
                m_serviceProvider->addServiceLazy([useAsync]
                {
                    return std::make_unique<TestServiceWithInit>(useAsync);
                });
            }
            else
            {
                m_serviceProvider->addService(std::make_unique<TestServiceWithInit>(useAsync));
            }
        }
    }

    TEST_F(TestService, addService_NonRefCounted)
    {
        m_serviceProvider->addService<TestService1>();
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface1>());
    }

    TEST_F(TestService, addService_RefCounted)
    {
        m_serviceProvider->addService<TestRCService12>();
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface2>());
    }

    TEST_F(TestService, addService_StdUniquePtr)
    {
        m_serviceProvider->addService(std::make_unique<TestService1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface1>());
    }

    TEST_F(TestService, addService_EastlUniquePtr)
    {
        m_serviceProvider->addService(std::make_unique<TestService1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface1>());
    }

    TEST_F(TestService, addService_NauPtr)
    {
        m_serviceProvider->addService(rtti::createInstance<TestRCService1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface1>());
    }

    TEST_F(TestService, addLazyService_StdUniquePtr)
    {
        m_serviceProvider->addServiceLazy([]
        {
            return std::make_unique<TestService12>();
        });
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface2>());
    }

    TEST_F(TestService, addLazyService_EastlUniquePtr)
    {
        m_serviceProvider->addServiceLazy([]
        {
            return std::make_unique<TestService12>();
        });
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface2>());
    }

    TEST_F(TestService, addLazyService_NauPtr)
    {
        m_serviceProvider->addService(rtti::createInstance<TestRCService1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface1>());

        m_serviceProvider->addServiceLazy([]
        {
            return rtti::createInstance<TestRCService12>();
        });
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface2>());
    }

    TEST_F(TestService, GetService)
    {
        m_serviceProvider->addService(std::make_unique<TestService12>());

        [[maybe_unused]]
        auto& s = m_serviceProvider->get<ITestInterface1>();
        ASSERT_THAT(m_serviceProvider->find<ITestInterface2>(), NotNull());
    }

    TEST_F(TestService, GetLazyApi)
    {
        m_serviceProvider->addServiceLazy([]
        {
            return std::make_unique<TestService1>();
        });

        m_serviceProvider->addServiceLazy([]
        {
            return std::make_unique<TestService12>();
        });

        const auto services1 = m_serviceProvider->getAll<ITestInterface1>();
        ASSERT_THAT(services1.size(), Eq(2));

        const auto services2 = m_serviceProvider->getAll<ITestInterface2>();
        ASSERT_THAT(services2.size(), Eq(1));
    }

    TEST_F(TestService, HasLazyApi)
    {
        bool fabricateAnyService = false;

        m_serviceProvider->addServiceLazy([&]
        {
            fabricateAnyService = true;
            return std::make_unique<TestService1>();
        });

        m_serviceProvider->addServiceLazy([&]
        {
            fabricateAnyService = true;
            return std::make_unique<TestService12>();
        });

        m_serviceProvider->addServiceLazy([&]
        {
            fabricateAnyService = true;
            return rtti::createInstance<TestRCService12>();
        });

        ASSERT_TRUE(m_serviceProvider->has<ITestInterface1>());
        ASSERT_TRUE(m_serviceProvider->has<ITestInterface2>());
        ASSERT_TRUE(m_serviceProvider->has<ITestRCInterface1>());

        ASSERT_FALSE(fabricateAnyService);
    }

    TEST_F(TestService, HasInitInterface)
    {
        auto* const serviceProviderInit = m_serviceProvider->as<core_detail::IServiceProviderInitialization*>();
        ASSERT_THAT(serviceProviderInit, NotNull());
    }

    TEST_F(TestService, FindSingleClass)
    {
        {
            auto classes = m_serviceProvider->findClasses<ITestInterface1>();
            ASSERT_TRUE(classes.empty());
        }

        m_serviceProvider->addClass<TestService1>();
        auto classes = m_serviceProvider->findClasses<ITestInterface1>();
        ASSERT_EQ(classes.size(), 1);
        ASSERT_EQ(classes.front()->getClassTypeInfo(), rtti::getTypeInfo<TestService1>());
    }

    TEST_F(TestService, FindClassesWithAnyInterface)
    {
        m_serviceProvider->addClass<TestService1>();
        m_serviceProvider->addClass<TestService1A2>();
        m_serviceProvider->addClass<TestService12>();
        m_serviceProvider->addClass<TestService3>();

        {
            // TestService1, TestService1A2, TestService12
            const auto classes = m_serviceProvider->findClasses<ITestInterface1, ITestInterface2>(true);
            ASSERT_EQ(classes.size(), 3);
            ASSERT_TRUE(containsClass<TestService1>(classes));
            ASSERT_TRUE(containsClass<TestService1A2>(classes));
            ASSERT_TRUE(containsClass<TestService12>(classes));
        }

        {
            // TestService1A2, TestService3
            auto classes = m_serviceProvider->findClasses<ITestInterface1A, ITestInterface3>(true);
            ASSERT_EQ(classes.size(), 2);
            ASSERT_TRUE(containsClass<TestService1A2>(classes));
            ASSERT_TRUE(containsClass<TestService3>(classes));
        }
    }

    TEST_F(TestService, FindClassesWithAllInterfaces)
    {
        m_serviceProvider->addClass<TestService1>();
        m_serviceProvider->addClass<TestService1A2>();
        m_serviceProvider->addClass<TestService12>();
        m_serviceProvider->addClass<TestService3>();

        {
            // TestService1A2, TestService12
            auto classes = m_serviceProvider->findClasses<ITestInterface1, ITestInterface2>(false);
            ASSERT_EQ(classes.size(), 2);

            ASSERT_TRUE(containsClass<TestService1A2>(classes));
            ASSERT_TRUE(containsClass<TestService12>(classes));
        }
    }

    TEST_P(TestServiceInit, PreInit)
    {
        registerAllServices();

        auto& serviceProviderInit = m_serviceProvider->as<core_detail::IServiceProviderInitialization&>();
        auto res = async::waitResult(serviceProviderInit.preInitServices());
        ASSERT_TRUE(res);

        const auto allServices = m_serviceProvider->getAll<ITestServiceInit>();
        ASSERT_THAT(allServices.size(), Eq(GetParam().serviceCount));

        const bool allArePreInitialized = std::all_of(allServices.begin(), allServices.end(), [](const ITestServiceInit* init)
        {
            return init->isPreInitialized();
        });

        const bool anyIsInitialized = std::any_of(allServices.begin(), allServices.end(), [](const ITestServiceInit* init)
        {
            return init->isInitialized();
        });
        ASSERT_TRUE(allArePreInitialized);
        ASSERT_FALSE(anyIsInitialized);
    }

    TEST_P(TestServiceInit, Init)
    {
        registerAllServices();

        auto& serviceProviderInit = m_serviceProvider->as<core_detail::IServiceProviderInitialization&>();
        auto res = async::waitResult(serviceProviderInit.initServices());
        ASSERT_TRUE(res);

        const auto allServices = m_serviceProvider->getAll<ITestServiceInit>();
        ASSERT_THAT(allServices.size(), Eq(GetParam().serviceCount));

        const bool anyIsPreInitialized = std::any_of(allServices.begin(), allServices.end(), [](const ITestServiceInit* init)
        {
            return init->isPreInitialized();
        });

        const bool allAreInitialized = std::all_of(allServices.begin(), allServices.end(), [](const ITestServiceInit* init)
        {
            return init->isInitialized();
        });
        ASSERT_FALSE(anyIsPreInitialized);
        ASSERT_TRUE(allAreInitialized);
    }

    TEST_P(TestServiceInit, PreInitAndInit)
    {
        registerAllServices();

        auto& serviceProviderInit = m_serviceProvider->as<core_detail::IServiceProviderInitialization&>();
        async::waitResult(serviceProviderInit.preInitServices()).ignore();
        async::waitResult(serviceProviderInit.initServices()).ignore();

        const auto allServices = m_serviceProvider->getAll<ITestServiceInit>();
        ASSERT_THAT(allServices.size(), Eq(GetParam().serviceCount));

        const bool allArePreInitialized = std::all_of(allServices.begin(), allServices.end(), [](const ITestServiceInit* init)
        {
            return init->isPreInitialized();
        });

        const bool allAreInitialized = std::all_of(allServices.begin(), allServices.end(), [](const ITestServiceInit* init)
        {
            return init->isInitialized();
        });
        ASSERT_TRUE(allArePreInitialized);
        ASSERT_TRUE(allAreInitialized);
    }

    INSTANTIATE_TEST_SUITE_P(
        Default,
        TestServiceInit,
        testing::ValuesIn(InitServiceTestData::getDefaults()));
}  // namespace nau::test