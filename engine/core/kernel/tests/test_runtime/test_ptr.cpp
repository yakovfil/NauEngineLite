// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_nau_ptr.cpp

#include <memory>

#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/weak_ptr.h"

namespace nau::test
{
    namespace
    {
        struct ITestInterface1 : virtual IRttiObject
        {
            NAU_INTERFACE(ITestInterface1, IRttiObject)
        };

        struct ITestInterface2 : virtual IRttiObject
        {
            NAU_INTERFACE(ITestInterface2, IRttiObject)
        };

        class TestService12 : public ITestInterface1,
                              public ITestInterface2
        {
            NAU_RTTI_CLASS(TestService12, ITestInterface1, ITestInterface2)
        };
    }  // namespace

    TEST(TestPtr, StdUniquePtrCast)
    {
        std::unique_ptr<ITestInterface1> itf1 = std::make_unique<TestService12>();

        static_assert(!std::is_constructible_v<std::unique_ptr<ITestInterface2>, decltype(itf1)>);

        auto itf2 = rtti::pointer_cast<ITestInterface2>(std::move(itf1));
        static_assert(std::is_same_v<std::unique_ptr<ITestInterface2>, decltype(itf2)>);

        ASSERT_TRUE(itf2);
        ASSERT_FALSE(itf1);

        static_assert(std::is_constructible_v<std::unique_ptr<IRttiObject>, decltype(itf2)>);

        auto itf3 = rtti::pointer_cast<IRttiObject>(std::move(itf2));
        static_assert(std::is_same_v<std::unique_ptr<IRttiObject>, decltype(itf3)>);

        ASSERT_TRUE(itf3);
        ASSERT_FALSE(itf2);
    }

    TEST(TestPtr, EastlUniquePtrCast)
    {
        std::unique_ptr<ITestInterface1> itf1 = std::make_unique<TestService12>();

        static_assert(!std::is_constructible_v<std::unique_ptr<ITestInterface2>, decltype(itf1)>);

        auto itf2 = rtti::pointer_cast<ITestInterface2>(std::move(itf1));
        static_assert(std::is_same_v<std::unique_ptr<ITestInterface2>, decltype(itf2)>);

        ASSERT_TRUE(itf2);
        ASSERT_FALSE(itf1);

        static_assert(std::is_constructible_v<std::unique_ptr<IRttiObject>, decltype(itf2)>);

        auto itf3 = rtti::pointer_cast<IRttiObject>(std::move(itf2));
        static_assert(std::is_same_v<std::unique_ptr<IRttiObject>, decltype(itf3)>);

        ASSERT_TRUE(itf3);
        ASSERT_FALSE(itf2);
    }
}  // namespace nau::test
