// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_nau_ptr.cpp

#include <concepts>
#include <string_view>

#include "nau/serialization/json_utils.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/string/string_utils.h"
#include "nau/utils/enum/enum_reflection.h"
#include "nau/utils/result.h"
#include "nau/utils/typed_flag.h"

namespace test_ns
{
    NAU_DEFINE_ENUM(
        TestEnumFlags,
        int,
        "test::TestEnumFlags",
        Flag0 = NauFlag(1),
        Flag1 = NauFlag(2),
        Flag3 = NauFlag(3))

}  // namespace test_ns

enum class TestEnumOld
{
    Value0,
    Value1,
    Value2 = 3,
    Value3,
    NoValue
};

NAU_DECLARE_ENUM(TestEnumOld, Value0, Value1, Value2, Value3)

namespace nau::test
{
    namespace
    {

        NAU_DEFINE_ENUM_(TestEnum,
                         Value0,
                         Value1,
                         Value3 = 3,
                         Value4,
                         Value10 = 10,
                         NoValue)

        NAU_DEFINE_ENUM_(
            ResourceReturnType,
            Unorm = 1,
            Snorm,
            Sint,
            Uint,
            Float,
            Mixed,
            Double,
            Continued)

        struct ObjectWithEnumField
        {
            std::vector<TestEnum> values;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(values))
        };

        struct ObjectWithOptionalEnumField
        {
            std::optional<TestEnum> value;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(value))
        };

    }  // namespace

    /**
        Test: enum items has expected values
    */
    TEST(TestEnumReflection, Definition)
    {
        ASSERT_EQ(static_cast<int>(TestEnum::Value0), 0);
        ASSERT_EQ(static_cast<int>(TestEnum::Value1), 1);
        ASSERT_EQ(static_cast<int>(TestEnum::Value3), 3);
        ASSERT_EQ(static_cast<int>(TestEnum::Value4), 4);
        ASSERT_EQ(static_cast<int>(TestEnum::Value10), 10);
        ASSERT_EQ(static_cast<int>(TestEnum::NoValue), 11);
    }

    /**
        Test: string to enum value conversion
    */
    TEST(TestEnumReflection, Parse)
    {
        auto toTestEnum = [](std::string_view str) -> TestEnum
        {
            TestEnum value{};
            parse(str, value).ignore();
            return value;
        };

        ASSERT_EQ(toTestEnum("Value0"), TestEnum::Value0);
        ASSERT_EQ(toTestEnum("Value1"), TestEnum::Value1);
        ASSERT_EQ(toTestEnum("VALUE3"), TestEnum::Value3);
        ASSERT_EQ(toTestEnum("value4"), TestEnum::Value4);
        ASSERT_EQ(toTestEnum("value10"), TestEnum::Value10);
        ASSERT_EQ(toTestEnum("novalue"), TestEnum::NoValue);
    }

    /**
        Test: enum to string conversion
    */
    TEST(TestEnumReflection, ToString)
    {
        ASSERT_EQ(toString(TestEnum::Value0), "Value0");
        ASSERT_EQ(toString(TestEnum::Value1), "Value1");
        ASSERT_EQ(toString(TestEnum::Value3), "Value3");
        ASSERT_EQ(toString(TestEnum::Value4), "Value4");
        ASSERT_EQ(toString(TestEnum::Value10), "Value10");
        ASSERT_EQ(toString(TestEnum::NoValue), "NoValue");
    }

    /**
        Test: defined enum has RuntimeValueRepresentation
     */
    TEST(TestEnumReflection, RuntimeValue_Representable)
    {
        static_assert(HasRuntimeValueRepresentation<TestEnum>);
    }

    TEST(TestEnumReflection, RuntimeValue_RepresentAsStringRef)
    {
        TestEnum enumValue = TestEnum::Value0;
        auto runtimeWrapper = makeValueRef(enumValue);
        ASSERT_TRUE(runtimeWrapper->is<RuntimeStringValue>());

        ASSERT_EQ(runtimeWrapper->getString(), "Value0");

        ASSERT_TRUE(runtimeWrapper->setString("value10").isSuccess());
        ASSERT_EQ(enumValue, TestEnum::Value10);
    }

    /**
        Test: parse enum values from json
    */
    TEST(TestEnumReflection, Serialization_ParseJson)
    {
        using namespace std::literals;

        std::string_view json = R"--(
            {
                "values": ["Value0", "Value1", "Value4", "Value10"]
            }

        )--"sv;

        auto obj = *serialization::JsonUtils::parse<ObjectWithEnumField>(json);
        ASSERT_EQ(obj.values.size(), 4);
        ASSERT_EQ(obj.values[0], TestEnum::Value0);
        ASSERT_EQ(obj.values[1], TestEnum::Value1);
        ASSERT_EQ(obj.values[2], TestEnum::Value4);
        ASSERT_EQ(obj.values[3], TestEnum::Value10);
    }

    /**
        Test: serialize enum values as json string
    */
    TEST(TestEnumReflection, Serialization_StoreJson)
    {
        using namespace std::literals;

        const ObjectWithEnumField obj1 = {
            .values = {TestEnum::Value4, TestEnum::Value0, TestEnum::Value10}
        };

        auto json = serialization::JsonUtils::stringify(obj1);
        auto obj2 = *serialization::JsonUtils::parse<ObjectWithEnumField>(json);
        ASSERT_EQ(obj2.values, obj1.values);
    }

    /**
     */
    TEST(TestEnumReflection, EnumRuntimeInfo)
    {
        using namespace std::literals;

        const IEnumRuntimeInfo& info = EnumTraits<TestEnum>::getRuntimeInfo();

        std::array expectedIntValues = {
            static_cast<int>(TestEnum::Value0),
            static_cast<int>(TestEnum::Value1),
            static_cast<int>(TestEnum::Value3),
            static_cast<int>(TestEnum::Value4),
            static_cast<int>(TestEnum::Value10),
            static_cast<int>(TestEnum::NoValue)};

        std::array expectedStrValues = {
            "Value0"sv,
            "Value1"sv,
            "Value3"sv,
            "Value4"sv,
            "Value10"sv,
            "NoValue"sv};

        ASSERT_EQ(info.getCount(), expectedIntValues.size());

        const auto values = info.getIntValues();
        ASSERT_EQ(values.size(), expectedIntValues.size());
        for (size_t i = 0; i < values.size(); ++i)
        {
            ASSERT_EQ(values[i], expectedIntValues[i]);
        }

        const auto strValues = info.getStringValues();
        ASSERT_EQ(strValues.size(), expectedStrValues.size());
        for (size_t i = 0; i < strValues.size(); ++i)
        {
            ASSERT_EQ(strValues[i], expectedStrValues[i]);
        }
    }

    /**
     */
    TEST(TestEnumReflection, ToStringAndParse)
    {
        const nau::IEnumRuntimeInfo& enumInfo = EnumTraits<ResourceReturnType>::getRuntimeInfo();

        ASSERT_EQ(enumInfo.getIntValues().size(), enumInfo.getStringValues().size());
        ASSERT_EQ(enumInfo.getIntValues().size(), 8);

        for (const int value : enumInfo.getIntValues())
        {
            const ResourceReturnType enumValue = static_cast<ResourceReturnType>(value);

            auto str = toString(enumValue);
            ASSERT_FALSE(str.empty());

            ResourceReturnType enumValue2 = ResourceReturnType::Continued;
            auto parseResult = parse(str, enumValue2);
            ASSERT_TRUE(parseResult);
            ASSERT_EQ(enumValue, enumValue2);
        }
    }

    /**
     */
    TEST(TestEnumReflection, SerializeOptionalValue)
    {
        using namespace nau::serialization;
        std::u8string_view json =
            u8R"--(
            {
                "value": ""
            }
        )--";

        Result<ObjectWithOptionalEnumField> parseResult = JsonUtils::parse<ObjectWithOptionalEnumField>(json);
        ASSERT_FALSE(parseResult);
    }

    /**

    */
    TEST(TestEnumReflection, BaseOld)
    {
        EXPECT_EQ(nau::string("Value0"), enum_to_str(TestEnumOld::Value0));
        EXPECT_EQ(nau::string("Value1"), enum_to_str(TestEnumOld::Value1));
        EXPECT_EQ(nau::string("Value2"), enum_to_str(TestEnumOld::Value2));
        EXPECT_EQ(nau::string("Value3"), enum_to_str(TestEnumOld::Value3));

        // TODO: Dubious operation: should return an error, not a string:
        EXPECT_EQ(nau::string("Unknown value for Enum: TestEnumOld"), enum_to_str(TestEnumOld::NoValue));

        EXPECT_EQ(nau::string::format(u8"{}", TestEnumOld::Value0), u8"Value0");
        EXPECT_EQ(nau::string::format(u8"{}", TestEnumOld::Value1), u8"Value1");
        EXPECT_EQ(nau::string::format(u8"{}", TestEnumOld::Value2), u8"Value2");
        EXPECT_EQ(nau::string::format(u8"{}", TestEnumOld::Value3), u8"Value3");

        TestEnumOld retVal;
        EXPECT_TRUE(str_to_enum(u8"Value0", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value0);
        EXPECT_TRUE(str_to_enum(u8"Value1", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value1);
        EXPECT_TRUE(str_to_enum(u8"Value2", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value2);
        EXPECT_TRUE(str_to_enum(u8"Value3", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value3);
        EXPECT_FALSE(str_to_enum(u8"NoValue", retVal));

        EXPECT_TRUE(str_to_enum("Value0", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value0);
        EXPECT_TRUE(str_to_enum("Value1", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value1);
        EXPECT_TRUE(str_to_enum("Value2", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value2);
        EXPECT_TRUE(str_to_enum("Value3", retVal));
        EXPECT_EQ(retVal, TestEnumOld::Value3);
        EXPECT_FALSE(str_to_enum("NoValue", retVal));
    }
}  // namespace nau::test
