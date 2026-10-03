// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <string_view>
#include <vector>

#include "nau/meta/class_info.h"
#include "nau/serialization/json.h"
#include "nau/serialization/json_utils.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"

using namespace ::testing;

namespace nau::test
{

    namespace
    {
        struct GenericData
        {
            int id = 0;
            std::string type;
            RuntimeValue::Ptr data1;
            RuntimeValue::Ptr data2;

#pragma region Class info
            NAU_CLASS_FIELDS(

                CLASS_FIELD(id),
                CLASS_FIELD(type),
                CLASS_FIELD(data1),
                CLASS_FIELD(data2))
#pragma endregion
        };

        struct DataWithTypeCoercion
        {
            NAU_CLASS_FIELDS(
                CLASS_FIELD(int64Field, serialization::TypeCoercion::Allow),
                CLASS_FIELD(strField, serialization::TypeCoercion::Allow))

            uint64_t int64Field = 0;
            std::string strField;
        };

        struct DataStrictTypeCoercion
        {
            NAU_CLASS_FIELDS(
                CLASS_FIELD(int64Field, serialization::TypeCoercion::Strict),
                CLASS_FIELD(strField, serialization::TypeCoercion::Strict))

            uint64_t int64Field = 0;
            std::string strField;
        };

        template <typename T>
        testing::AssertionResult checkPrimitive(T value, std::string_view expectedStr)
        {
            const std::u8string text = serialization::JsonUtils::stringify(value);
            if (!strings::icaseEqual(strings::toStringView(text), expectedStr))
            {
                return testing::AssertionFailure() << ::fmt::format("Invalid json string:({}), expected:({})", strings::toStringView(text), expectedStr);
            }

            const Result<const T> parsedValue = serialization::JsonUtils::parse<T>(text);
            if (!parsedValue)
            {
                return testing::AssertionFailure() << strings::toStringView(parsedValue.getError()->getMessage());
            }

            if (*parsedValue != value)
            {
                return testing::AssertionFailure() << ::fmt::format("Invalid json parse value on type:({})", testing::internal::GetTypeName<T>());
            }

            return testing::AssertionSuccess();
        }

    }  // namespace

    /**
     */
    TEST(TestSerializationJson, CreateDictionary)
    {
        RuntimeDictionary::Ptr value = serialization::jsonCreateDictionary();
        ASSERT_TRUE(value);
        ASSERT_EQ(value->as<serialization::JsonValueHolder&>().getThisJsonValue().type(), Json::ValueType::objectValue);
    }

    /**
     */
    TEST(TestSerializationJson, CreateCollection)
    {
        RuntimeCollection::Ptr value = serialization::jsonCreateCollection();
        ASSERT_TRUE(value);
        ASSERT_EQ(value->as<serialization::JsonValueHolder&>().getThisJsonValue().type(), Json::ValueType::arrayValue);
    }

    /**
     */
    TEST(TestSerializationJson, JsonObjectToRuntimeValue)
    {
        using namespace nau::serialization;

        const std::string_view jsonStr = R"--(
            {
                "id": 111,
                "type": "object",
            }
        )--";

        const RuntimeDictionary::Ptr value = jsonToRuntimeValue(*jsonParseToValue(jsonStr));
        ASSERT_TRUE(value);
        ASSERT_TRUE(value->containsKey("id"));
        ASSERT_TRUE(value->containsKey("type"));
    }

    /**
     */
    TEST(TestSerializationJson, JsonArrayToRuntimeValue)
    {
        using namespace nau::serialization;

        const std::string_view jsonStr = R"--(
            [1, 2, true, 77]
        )--";

        const RuntimeCollection::Ptr value = jsonToRuntimeValue(*jsonParseToValue(jsonStr));
        ASSERT_TRUE(value);
        ASSERT_EQ(value->getSize(), 4);
    }

    /**
     */
    TEST(TestSerializationJson, JsonIntToRuntimeValue)
    {
        const RuntimeIntegerValue::Ptr value = serialization::jsonToRuntimeValue(Json::Value{77});
        ASSERT_EQ(value->getInt64(), int64_t{77});
    }

    /**
     */
    TEST(TestSerializationJson, JsonFloatToRuntimeValue)
    {
        const RuntimeFloatValue::Ptr value = serialization::jsonToRuntimeValue(Json::Value{77.7f});
        ASSERT_EQ(value->getSingle(), 77.7f);
    }

    /**
     */
    TEST(TestSerializationJson, JsonStringToRuntimeValue)
    {
        const RuntimeStringValue::Ptr value = serialization::jsonToRuntimeValue(Json::Value{"text"});
        ASSERT_EQ(value->getString(), std::string_view{"text"});
    }

    /**
     */
    TEST(TestSerializationJson, JsonBoolToRuntimeValue)
    {
        const RuntimeBooleanValue::Ptr value = serialization::jsonToRuntimeValue(Json::Value{true});
        ASSERT_TRUE(value->getBool());
    }

    /**
     */
    TEST(TestSerializationJson, JsonWrapObject)
    {
        Json::Value jsonValue{Json::ValueType::objectValue};
        jsonValue["field1"] = 111;

        {
            const RuntimeDictionary::Ptr dict = serialization::jsonAsRuntimeValue(jsonValue);
            ASSERT_TRUE(dict->containsKey("field1"));
            dict->setValue("field2", makeValueCopy(222)).ignore();
        }

        ASSERT_EQ(jsonValue["field2"].asInt(), 222);
    }

    /**
     */
    TEST(TestSerializationJson, JsonWrapCollection)
    {
        Json::Value jsonValue{Json::ValueType::arrayValue};
        jsonValue.append(111);
        jsonValue.append(222);

        {
            const RuntimeCollection::Ptr collection = serialization::jsonAsRuntimeValue(jsonValue);
            ASSERT_EQ(collection->getSize(), 2);
            ASSERT_EQ((*collection)[0]->as<const RuntimeIntegerValue&>().getInt64(), 111);
            ASSERT_EQ((*collection)[1]->as<const RuntimeIntegerValue&>().getInt64(), 222);

            collection->append(makeValueCopy(std::string_view{"text"})).ignore();
            collection->append(makeValueCopy(444.4f)).ignore();
        }

        ASSERT_EQ(jsonValue.size(), 4);
        ASSERT_EQ(jsonValue[2].type(), Json::ValueType::stringValue);
        ASSERT_EQ(jsonValue[3].type(), Json::ValueType::realValue);
    }

    /**
     */
    TEST(TestSerializationJson, ReadWritePrimitive)
    {
        EXPECT_TRUE(checkPrimitive(static_cast<uint16_t>(10), "10"));
        EXPECT_TRUE(checkPrimitive(static_cast<int32_t>(-236), "-236"));
        EXPECT_TRUE(checkPrimitive((double)101.75, "101.75"));
        EXPECT_TRUE(checkPrimitive(true, "true"));
        EXPECT_TRUE(checkPrimitive(false, "false"));
        EXPECT_TRUE(checkPrimitive(std::string{"abc"}, "\"abc\""));
    }

    /**
     */
    TEST(TestSerializationJson, StringifyCollection)
    {
        std::vector<unsigned> ints = {1, 2, 3, 4, 5};

        const auto str = serialization::JsonUtils::stringify(ints);
        ASSERT_EQ(str, u8"[1,2,3,4,5]");
    }

    /**
     */
    TEST(TestSerializationJson, StringifyObject)
    {
        GenericData data;
        data.id = 100;
        data.type = "unknown";
        data.data1 = makeValueCopy(std::vector<unsigned>{1, 2, 3});
        const auto str = serialization::JsonUtils::stringify(data);
    }

    /**
     */
    TEST(TestSerializationJson, ParseObject)
    {
        std::u8string_view json =
            u8R"--(
            {
                "id": 222,
                "type": "object",
                "data1": {
                    "id": 101,
                    "type": "number",
                    "data1": 100,
                    "data2": 200
                }
            }
        )--";

        auto value2 = serialization::JsonUtils::parse<GenericData>(json);
        ASSERT_TRUE(value2);

        const auto field21 = runtimeValueCast<GenericData>(value2->data1);
        ASSERT_TRUE(field21);

        ASSERT_THAT(*runtimeValueCast<int>(field21->data1), Eq(100));
        ASSERT_THAT(*runtimeValueCast<int>(field21->data2), Eq(200));
    }

    TEST(TestSerializationJson, TypeCoercion1)
    {
        std::u8string_view json1 =
            u8R"--(
            {
                "int64Field": "12345678",
                "strField": 976854
            }
        )--";

        const auto value1 = serialization::JsonUtils::parse<DataWithTypeCoercion>(json1);
        ASSERT_TRUE(value1);

        ASSERT_THAT(value1->int64Field, Eq(12345678));
        ASSERT_THAT(value1->strField, Eq("976854"));
    }

    TEST(TestSerializationJson, TypeCoercion2)
    {
        std::u8string_view json1 =
            u8R"--(
            {
                "int64Field": ""
            }
        )--";

        DataWithTypeCoercion value1;
        value1.int64Field = 12345;
        value1.strField = "text";

        serialization::JsonUtils::parse(value1, json1).ignore();

        ASSERT_THAT(value1.int64Field, Eq(0));
    }

    // TODO NAU-2089
    /*
    TEST(TestSerializationJson, StrictTypeCoercion1)
    {
        GTEST_SKIP_("TypeCoercion::Strict is not implemented.");

        std::u8string_view json1 =
            u8R"--(
            {
                "int64Field": "12345678",
                "strField": 976854
            }
        )--";

        const auto value1 = serialization::JsonUtils::parse<DataStrictTypeCoercion>(json1);
        ASSERT_FALSE(value1);
    }
    */

    /**
        Test: accessing field through string_view (when string view 'points' on middle of the string)
     */
    TEST(TestSerializationJson, StringViewKey)
    {
        std::string_view json =
            R"--(
            {
                "id": 222,
                "type": "object",
                "data1": {
                    "id": 101,
                    "type": "number",
                    "data1": 100,
                    "data2": 200
                }
            }
        )--";

        std::string_view fieldNames = "id, type, data1";

        RuntimeDictionary::Ptr dict = *serialization::jsonParseString(json);
        dict->getValue("boo");

        for (auto field : strings::split(fieldNames, std::string_view{","}))
        {
            field = strings::trim(field);
            ASSERT_TRUE(dict->containsKey(field));
            auto fieldValue = dict->getValue(field);

            if (auto intValue = fieldValue->as<RuntimeIntegerValue*>())
            {
                ASSERT_EQ(intValue->getInt64(), 222);
                dict->setValue(field, makeValueCopy(333)).ignore();
            }
            else if (auto strValue = fieldValue->as<RuntimeStringValue*>())
            {
                ASSERT_EQ(strValue->getString(), "object");
                dict->setValue(field, makeValueCopy(std::string{"array"})).ignore();
            }
        }

        const Json::Value& jsonValue = dict->as<const serialization::JsonValueHolder&>().getThisJsonValue();

        ASSERT_EQ(jsonValue["id"].asInt(), 333);
        ASSERT_EQ(jsonValue["type"].asString(), "array");
    }

}  // namespace nau::test
