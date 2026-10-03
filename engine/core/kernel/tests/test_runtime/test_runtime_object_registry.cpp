// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <span>
#include <vector>

#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_object_registry.h"

namespace nau::test
{
    class TestRuntimeObjectRegistry : public ::testing::Test
    {
    protected:
        TestRuntimeObjectRegistry()
        {
            RuntimeObjectRegistry::setDefaultInstance();
        }

        ~TestRuntimeObjectRegistry()
        {
            RuntimeObjectRegistry::releaseInstance();
        }

        static testing::AssertionResult hasNoRegisteredObjects()
        {
            const size_t counter = getRegisteredObjectCount();
            if (counter == 0)
            {
                return testing::AssertionSuccess();
            }

            return testing::AssertionFailure() << std::format("Has ({}) registered objects", counter);
        }

        static size_t getRegisteredObjectCount()
        {
            size_t counter = 0;

            if (RuntimeObjectRegistry::hasInstance())
            {
                RuntimeObjectRegistry::getInstance().visitAllObjects([&counter](std::span<IRttiObject*> objects)
                {
                    counter = objects.size();
                });
            }

            return counter;
        }
    };

    namespace
    {
        class UniqueType : public virtual IRttiObject
        {
            NAU_RTTI_CLASS(UniqueType, IRttiObject)
        };

        class AutoType : public virtual IRefCounted
        {
            NAU_CLASS_(AutoType, IRefCounted)
        };

        class DisposableHelper : public IDisposable,
                                 public virtual IRefCounted
        {
            NAU_CLASS_(DisposableHelper, IDisposable, IRefCounted)

        public:
            DisposableHelper() :
                m_registration{*this}
            {
            }

            ~DisposableHelper()
            {
            }

            void dispose() override
            {
                m_registration = nullptr;
            }

        private:
            RuntimeObjectRegistration m_registration;
        };

    }  // namespace

    TEST(TestRuntimeObjectRegistry_1, AccessNoInstance)
    {
        auto object = std::make_unique<UniqueType>();
        RuntimeObjectRegistration reg{*object};
        reg = nullptr;
    }

    TEST_F(TestRuntimeObjectRegistry, InitRelease)
    {
    }

    TEST_F(TestRuntimeObjectRegistry, SimpleRegisterUnregisterRttiObject)
    {
        {
            auto object = std::make_unique<UniqueType>();
            const RuntimeObjectRegistration reg{*object};
            ASSERT_EQ(getRegisteredObjectCount(), 1);
        }

        ASSERT_TRUE(hasNoRegisteredObjects());
    }

    TEST_F(TestRuntimeObjectRegistry, RegisterRefCountedAsRttiObject)
    {
        auto object = rtti::createInstance<AutoType>();
        RuntimeObjectRegistration reg{*object};
        ASSERT_EQ(getRegisteredObjectCount(), 1);

        reg = nullptr;
    }

    TEST_F(TestRuntimeObjectRegistry, SimpleRegisterUnregisterRefCounted)
    {
        {
            auto object = rtti::createInstance<AutoType>();
            const RuntimeObjectRegistration reg{object};
            ASSERT_EQ(getRegisteredObjectCount(), 1);
        }

        ASSERT_TRUE(hasNoRegisteredObjects());
    }

    TEST_F(TestRuntimeObjectRegistry, AutoRemove)
    {
        {
            auto object = rtti::createInstance<AutoType>();
            RuntimeObjectRegistration{object}.setAutoRemove();

            auto object2 = std::make_unique<UniqueType>();
            const RuntimeObjectRegistration reg{*object2};

            ASSERT_EQ(getRegisteredObjectCount(), 2);

            object.reset();
            ASSERT_EQ(getRegisteredObjectCount(), 1);
        }

        ASSERT_TRUE(hasNoRegisteredObjects());
    }

    TEST_F(TestRuntimeObjectRegistry, AutoRemove2)
    {
        auto object = rtti::createInstance<AutoType>();
        RuntimeObjectRegistration reg{object};

        ASSERT_EQ(getRegisteredObjectCount(), 1);
        object.reset();  // registration object still alive
        ASSERT_TRUE(hasNoRegisteredObjects());
    }

    TEST_F(TestRuntimeObjectRegistry, VisitAll)
    {
        std::vector<std::unique_ptr<UniqueType>> objects1;
        std::vector<nau::Ptr<>> objects2;
        std::vector<nau::RuntimeObjectRegistration> registrations;

        for (size_t i = 0; i < 5; ++i)
        {
            objects1.push_back(std::make_unique<UniqueType>());
            registrations.push_back(*objects1.back());

            objects2.push_back(rtti::createInstance<AutoType>());
            registrations.push_back(*objects2.back());
        }

        size_t counter = 0;
        RuntimeObjectRegistry::getInstance().visitAllObjects([&counter](std::span<IRttiObject*> objects)
        {
            counter = objects.size();
        });

        ASSERT_EQ(objects1.size() + objects2.size(), counter);

        objects2.clear();
    }

    TEST_F(TestRuntimeObjectRegistry, UnregisterFromVisit)
    {
        auto helper = rtti::createInstance<DisposableHelper>();

        RuntimeObjectRegistry::getInstance().visitObjects<IDisposable>([](std::span<IRttiObject*> objects)
        {
            for (auto* obj : objects)
            {
                obj->as<IDisposable&>().dispose();
            }
        });
    }

}  // namespace nau::test
