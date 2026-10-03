// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/meta/attribute.h"

using namespace testing;

namespace nau::test
{
    namespace
    {
        NAU_DEFINE_ATTRIBUTE_(TestAttrib0);
        NAU_DEFINE_ATTRIBUTE_(TestAttrib1);
        NAU_DEFINE_ATTRIBUTE_(TestAttrib2);
        NAU_DEFINE_ATTRIBUTE_(TestAttrib3);
        NAU_DEFINE_ATTRIBUTE_(TestAttribX);
        NAU_DEFINE_ATTRIBUTE_(TestAttribExt1);
        NAU_DEFINE_ATTRIBUTE_(TestAttribExt2);

        NAU_DEFINE_ATTRIBUTE(AttribInherit0, "", meta::AttributeOptions::Inherited)
        NAU_DEFINE_ATTRIBUTE(AttribInherit1, "", meta::AttributeOptions::Inherited)


        template <typename T>
        struct AttributeValue
        {
            using type = T;
        };

        class MyType0
        {
        };

        class MyType1
        {
            NAU_CLASS_ATTRIBUTES(
                CLASS_ATTRIBUTE(TestAttrib0, std::string_view{"Attrib0"}),
                CLASS_ATTRIBUTE(TestAttrib1, 75),
                CLASS_ATTRIBUTE(AttribInherit0, std::string_view{"tag_0"}),
                CLASS_ATTRIBUTE(TestAttrib2, AttributeValue<float>{}),
                CLASS_ATTRIBUTE(AttribInherit1, std::string_view{"tag_1"})
            )
        };

        NAU_CLASS_ATTRIBUTES_EXT(MyType1,
                                 CLASS_ATTRIBUTE(TestAttribExt1, 10),
                                 CLASS_ATTRIBUTE(TestAttribExt2, 20))

        class MyType2
        {
        };

        NAU_CLASS_ATTRIBUTES_EXT(MyType2,
                                 CLASS_ATTRIBUTE(TestAttrib0, std::string_view{"Attrib0"}),
                                 CLASS_ATTRIBUTE(TestAttrib1, 75))

        class MyType_Inherit1 : public MyType1
        {
            NAU_CLASS_BASE(MyType1)

           NAU_CLASS_ATTRIBUTES(
                CLASS_ATTRIBUTE(TestAttrib3, std::string_view{"Attrib3"})
           )
        };

    }  // namespace

    /**
     */
    TEST(TestAttribute, AttributeDefined)
    {
        static_assert(meta::AttributeDefined<MyType1, TestAttrib0>);
        static_assert(meta::AttributeDefined<MyType1, TestAttrib1>);
        static_assert(meta::AttributeDefined<MyType1, TestAttrib2>);
    }

    /**
     */
    TEST(TestAttribute, AttributeDefined_Ext)
    {
        static_assert(meta::AttributeDefined<MyType1, TestAttribExt1>);
        static_assert(meta::AttributeDefined<MyType1, TestAttribExt1>);

        static_assert(meta::AttributeDefined<MyType1, TestAttrib0>);
        static_assert(meta::AttributeDefined<MyType1, TestAttrib1>);
    }

    /**
     */
    TEST(TestAttribute, AttributeNotDefined)
    {
        static_assert(!meta::AttributeDefined<MyType0, TestAttribX>);
        static_assert(!meta::AttributeDefined<MyType1, TestAttribX>);
        static_assert(!meta::AttributeDefined<MyType2, TestAttribX>);
    }

    /**
     */
    TEST(TestAttribute, AttributeNotDefinedForClassWithNoAttribs)
    {
        static_assert(!meta::AttributeDefined<MyType0, TestAttrib0>);
        static_assert(!meta::AttributeDefined<MyType0, TestAttrib1>);
        static_assert(!meta::AttributeDefined<MyType0, TestAttrib2>);
    }

    /**
     */
    TEST(TestAttribute, AttributeValueType)
    {
        static_assert(std::is_same_v<meta::AttributeValueType<MyType1, TestAttrib0>, std::string_view>);
        static_assert(std::is_integral_v<meta::AttributeValueType<MyType1, TestAttrib1>>);
        static_assert(std::is_same_v<meta::AttributeValueType<MyType1, TestAttrib2>::type, float>);
    }

    /**
     */
    TEST(TestAttribute, AttributeValue)
    {
        auto value_0 = meta::getAttributeValue<MyType1, TestAttrib0>();
        ASSERT_EQ(value_0, "Attrib0");

        auto value_1 = meta::getAttributeValue<MyType1, TestAttrib1>();
        ASSERT_EQ(value_1, 75);

        auto value_2 = meta::getAttributeValue<MyType1, TestAttrib2>();
        static_assert(std::is_same_v<decltype(value_2)::type, float>);
    }

    TEST(TestAttribute, AttributeByDefaultNotInherited)
    {
        static_assert(!meta::AttributeDefined<MyType_Inherit1, TestAttrib0>);
        static_assert(!meta::AttributeDefined<MyType_Inherit1, TestAttrib1>);
        static_assert(!meta::AttributeDefined<MyType_Inherit1, TestAttrib2>);
        static_assert(!meta::AttributeDefined<MyType_Inherit1, TestAttribExt1>);
        static_assert(!meta::AttributeDefined<MyType_Inherit1, TestAttribExt2>);
    }

    TEST(TestAttribute, InheritedAttributes)
    {
       static_assert(meta::AttributeDefined<MyType_Inherit1, AttribInherit0>);
       static_assert(meta::AttributeDefined<MyType_Inherit1, AttribInherit1>);
       static_assert(meta::AttributeDefined<MyType_Inherit1, TestAttrib3>);
    }

    TEST(TestAttribute, InheritedAttributesValue)
    {
        auto value_0 = meta::getAttributeValue<MyType_Inherit1, AttribInherit0>();
        ASSERT_EQ(value_0, "tag_0");

        auto value_1 = meta::getAttributeValue<MyType_Inherit1, AttribInherit1>();
        ASSERT_EQ(value_1, "tag_1");
    }
}  // namespace nau::test
