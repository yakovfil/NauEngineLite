// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/type_info.h"

namespace nau::test
{
    struct MyTypeWithTypeId
    {
        NAU_TYPEID(MyTypeWithTypeId);
    };

    struct MyTypeWithTypeId2
    {
    };

    struct MyTypeNoTypeId
    {
    };

}  // namespace nau::test

NAU_DECLARE_TYPEID(nau::test::MyTypeWithTypeId2)

namespace nau::test
{
    TEST(TestTypeInfo, ConstructFromLiteralOnly)
    {
        [[maybe_unused]] const std::string text = "TypeName";
        [[maybe_unused]] const char charLiteral[9] = "TypeName";

        static_assert(std::is_constructible_v<nau::rtti_detail::TypeId, decltype(charLiteral)>);
        static_assert(!std::is_constructible_v<nau::rtti_detail::TypeId, decltype(text.c_str())>);
    }

    TEST(TestTypeInfo, HasTypeInfo)
    {
        static_assert(rtti::HasTypeInfo<MyTypeWithTypeId>);
        static_assert(rtti::HasTypeInfo<MyTypeWithTypeId2>);
        static_assert(!rtti::HasTypeInfo<MyTypeNoTypeId>);
    }

    TEST(TestTypeInfo, TypeIdIsConstexpr)
    {
        constexpr rtti_detail::TypeId typeId{"NAME"};
        static_assert(typeId.typeId != 0);
    }

    TEST(TestTypeInfo, GetTypeInfo)
    {
        ASSERT_NE(rtti::getTypeInfo<MyTypeWithTypeId>().getHashCode(), 0);
        ASSERT_NE(rtti::getTypeInfo<MyTypeWithTypeId2>().getHashCode(), 0);
        ASSERT_NE(rtti::getTypeInfo<MyTypeWithTypeId2>().getHashCode(), rtti::getTypeInfo<MyTypeWithTypeId>().getHashCode());
    }

    TEST(TestTypeInfo, MakeFromId)
    {
        const rtti::TypeInfo& typeInfo = rtti::getTypeInfo<MyTypeWithTypeId>();
        const auto tId = typeInfo.getHashCode();

        rtti::TypeInfo typeInfo2 = rtti::makeTypeInfoFromId(tId);
        ASSERT_EQ(typeInfo2, typeInfo);
    }

    TEST(TestTypeInfo, MakeFromTypeName)
    {
        const rtti::TypeInfo& typeInfo = rtti::getTypeInfo<MyTypeWithTypeId>();
        const auto typeName = typeInfo.getTypeName();

        rtti::TypeInfo typeInfo2 = rtti::makeTypeInfoFromName(std::string{typeName}.c_str());
        ASSERT_EQ(typeInfo2, typeInfo);
    }
}  // namespace nau::test
