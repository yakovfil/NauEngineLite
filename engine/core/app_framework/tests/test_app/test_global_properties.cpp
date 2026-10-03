// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "nau/app/global_properties.h"
#include "nau/io/memory_stream.h"

namespace nau
{
    // defined in: engine/core/app_framework/src/app/global_properties_impl.cpp
    std::unique_ptr<GlobalProperties> createGlobalProperties();
}  // namespace nau

namespace nau::test
{
    namespace
    {
        struct PropSectionData
        {
            struct SubSection
            {
                float propFloat = 0.f;
                std::string propText;

                NAU_CLASS_FIELDS(
                    CLASS_NAMED_FIELD(propFloat, "prop_float"),
                    CLASS_NAMED_FIELD(propText, "prop_text"), )
            };

            bool propBoolTrue = false;
            bool propBoolFalse = false;
            SubSection subSection;

            NAU_CLASS_FIELDS(
                CLASS_NAMED_FIELD(propBoolTrue, "prop_bool_true"),
                CLASS_NAMED_FIELD(propBoolFalse, "prop_bool_false"),
                CLASS_NAMED_FIELD(subSection, "section_021"))
        };
    }  // namespace

    class TestGlobalProperties : public testing::Test
    {
    protected:
        static std::string_view getPropsJson()
        {
            return R"-(
                {
                    "prop_int1": 1,
                    "prop_int2": 2,
                    "prop_str": "text",
                    "section_0" :
                    {
                        "prop0": 100,
                        "section_01":
                        {

                        },
                        "section_02":
                        {
                            "prop_bool_true": true,
                            "prop_bool_false": false,
                            "section_021":
                            {
                                "prop_float": 77.0,
                                "prop_text": "section_021"
                            }
                        },
                    }
                })-";
        }

        static testing::AssertionResult mergeFromJson(GlobalProperties& props, std::string_view json)
        {
            auto stream = io::createReadonlyMemoryStream({reinterpret_cast<const std::byte*>(json.data()), json.size()});
            if (auto parseRes = mergePropertiesFromStream(props, *stream, "application/json"); !parseRes)
            {
                return testing::AssertionFailure() << parseRes.getError()->getMessage().c_str();
            }

            return testing::AssertionSuccess();
        }

        std::string dumpToString() const
        {
            const std::string str = dumpPropertiesToString(*m_props);
            return {str.data(), str.size()};
        }

        const std::unique_ptr<GlobalProperties> m_props = createGlobalProperties();
    };

    /**
     */
    TEST_F(TestGlobalProperties, CreateServiceInstance)
    {
        ASSERT_TRUE(m_props);
    }

    /**
     */
    TEST_F(TestGlobalProperties, ReadWhenEmpty)
    {
        ASSERT_FALSE(m_props->contains("prop_int1"));

        const std::optional<unsigned> value = m_props->getValue<unsigned>("prop_int2");
        ASSERT_FALSE(value);
    }

    /**
     */
    TEST_F(TestGlobalProperties, ReadPropsFromJson)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));

        ASSERT_TRUE(m_props->contains("prop_int1"));
        ASSERT_TRUE(m_props->contains("prop_int2"));
        ASSERT_TRUE(m_props->contains("prop_str"));
        ASSERT_TRUE(m_props->contains("section_0"));
    }

    /**
     */
    TEST_F(TestGlobalProperties, GetSimpleValue)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));

        ASSERT_EQ(m_props->getValue<int>("prop_int1"), 1);
        ASSERT_EQ(m_props->getValue<int>("prop_int2"), 2);
        ASSERT_EQ(m_props->getValue<std::string>("prop_str"), std::string{"text"});
    }

    /**
     */
    TEST_F(TestGlobalProperties, GetValueAtPath)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));

        ASSERT_TRUE(m_props->contains("section_0/prop0"));
        ASSERT_TRUE(m_props->contains("section_0/section_01"));
        ASSERT_TRUE(m_props->contains("section_0/section_02"));
        ASSERT_TRUE(m_props->contains("section_0/section_02/section_021"));
        ASSERT_TRUE(m_props->contains("section_0/section_02/section_021/prop_float"));
        ASSERT_TRUE(m_props->contains("section_0/section_02/section_021/prop_text"));

        std::optional<float> value = m_props->getValue<float>("section_0/section_02/section_021/prop_float");
        ASSERT_TRUE(value);
        ASSERT_EQ(*value, 77.f);
    }

    /**
        Test:
            access property that contains root '/'
     */
    TEST_F(TestGlobalProperties, GetWithRoot)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));
        ASSERT_EQ(m_props->getValue<int>("/prop_int1"), 1);
    }

    /**
     */
    TEST_F(TestGlobalProperties, ReadCompoundValue)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));
        std::optional<PropSectionData> value = m_props->getValue<PropSectionData>("section_0/section_02");

        ASSERT_TRUE(value);
        ASSERT_EQ(value->propBoolTrue, true);
        ASSERT_EQ(value->propBoolFalse, false);
        ASSERT_EQ(value->subSection.propFloat, 77.f);
        ASSERT_EQ(value->subSection.propText, std::string{"section_021"});
    }

    /**
     */
    TEST_F(TestGlobalProperties, InvalidPropAccess)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));

        auto value = m_props->getValue<float>("prop_int1/invalid_value");
        ASSERT_FALSE(value);
    }

    /**
     */
    TEST_F(TestGlobalProperties, SetPrimitiveValue)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));
        ASSERT_FALSE(m_props->contains("/rootPropInt"));
        ASSERT_FALSE(m_props->contains("/rootPropInt2"));
        ASSERT_FALSE(m_props->contains("/section_0/propInt"));

        m_props->setValue("rootPropInt", 75).ignore();
        m_props->setValue("/rootPropInt2", 88).ignore();
        m_props->setValue("section_0/propInt", 775).ignore();

        ASSERT_TRUE(m_props->contains("/rootPropInt"));
        ASSERT_TRUE(m_props->contains("/rootPropInt2"));
        ASSERT_TRUE(m_props->contains("/section_0/propInt"));
        ASSERT_EQ(m_props->getValue<int>("/rootPropInt"), 75);
        ASSERT_EQ(m_props->getValue<int>("/rootPropInt2"), 88);
        ASSERT_EQ(m_props->getValue<int>("section_0/propInt"), 775);
    }

    /**
        Test:
            Setting property will attempt to create whole path (for elements that does not yet exists).
            But if some existing element is not directory then setting property must fail.
     */
    TEST_F(TestGlobalProperties, SetPropertyInvalidPath)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));
        ASSERT_TRUE(m_props->contains("/section_0/prop0"));

        Result<> setResult = m_props->setValue("/section_0/prop0/prop1", 75);
        ASSERT_FALSE(setResult);
    }

    /**
        Test:
            setValue for collection property actually should merge incoming values with existing one.
     */
    TEST_F(TestGlobalProperties, SetCollection)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));

        const std::string_view propName = "/section_0/section_01/ids";

        m_props->setValue(propName, std::vector<unsigned>{100, 200, 300}).ignore();
        m_props->setValue(propName, std::vector<unsigned>{400, 500, 600}).ignore();

        auto ids = *m_props->getValue<std::vector<unsigned>>(propName);
        ASSERT_EQ(ids.size(), 6);
        ASSERT_EQ(ids[0], 100);
        ASSERT_EQ(ids[1], 200);
        ASSERT_EQ(ids[2], 300);
        ASSERT_EQ(ids[3], 400);
        ASSERT_EQ(ids[4], 500);
        ASSERT_EQ(ids[5], 600);
    }

    /**
        Test:
            test getModify
     */
    TEST_F(TestGlobalProperties, GetModify)
    {
        const std::string_view propName = "/section_0/section_01/ids";

        m_props->setValue(propName, std::vector<unsigned>{100, 200, 300}).ignore();

        {
            GlobalProperties::ModificationLock lock;

            Result<RuntimeCollection::Ptr> collection = *m_props->getModify(propName, lock);
            ASSERT_TRUE(collection);

            (*collection)->clear();
            RuntimeValue::assign(*collection, makeValueCopy(std::vector<unsigned>{400, 500, 600})).ignore();
        }

        auto ids = *m_props->getValue<std::vector<unsigned>>(propName);
        ASSERT_EQ(ids.size(), 3);
        ASSERT_EQ(ids[0], 400);
        ASSERT_EQ(ids[1], 500);
        ASSERT_EQ(ids[2], 600);
    }

    /**
        Test:
            getModify will return mutable object only if target property exists and it is collection (or dictionary)
    */
    TEST_F(TestGlobalProperties, GetModifyForNonCollectionIsFailed)
    {
        const std::string_view propName = "/section_0/section_01/ids";

        m_props->setValue(propName, 777).ignore();

        std::unique_lock<std::shared_mutex> lock;
        Result<RuntimeValue::Ptr> collection = m_props->getModify(propName, lock);
        ASSERT_FALSE(collection);
    }

    /**
        Test:
            getModify for non existent path must fail
    */
    TEST_F(TestGlobalProperties, GetModifyForInvalidPathIsFailed)
    {
        ASSERT_TRUE(mergeFromJson(*m_props, getPropsJson()));
        const std::string_view propName = "/section_0/section_non_existent";
        ASSERT_FALSE(m_props->contains(propName));

        std::unique_lock<std::shared_mutex> lock;
        Result<RuntimeValue::Ptr> collection = m_props->getModify(propName, lock);
        ASSERT_FALSE(collection);
    }

    /**
        Test:
    */
    TEST_F(TestGlobalProperties, ExpandProperty)
    {
        m_props->setValue("/name_1", std::string{"Name1Value"}).ignore();
        m_props->setValue("/prop1", std::string{"Name:${/name_1}"}).ignore();

        auto str = *m_props->getValue<std::string>("/prop1");
        ASSERT_EQ(str, "Name:Name1Value");
    }

    /**
        Test:
    */
    TEST_F(TestGlobalProperties, ExpandPropertyEmpty)
    {
        m_props->setValue("/prop1", std::string{"Name:${/name_1}"}).ignore();
        auto str = *m_props->getValue<std::string>("/prop1");
        ASSERT_EQ(str, "Name:");
    }

    /**
        Test: variable custom value resolver
     */
    TEST_F(TestGlobalProperties, CustomVarResolver)
    {
        m_props->addVariableResolver("test1", [](std::string_view inStr) -> std::optional<std::string>
        {
            if (inStr == "value1")
            {
                return std::string{"AAA"};
            }

            return std::nullopt;
        });

        m_props->addVariableResolver("test2", [](std::string_view inStr) -> std::optional<std::string>
        {
            if (inStr == "value2")
            {
                return std::string{"BBB"};
            }

            return std::nullopt;
        });

        m_props->setValue("prop1/field1", std::string{"$test1{value1},$test1{value2}"}).ignore();
        m_props->setValue("prop2/field1", std::string{"$test2{value1},$test2{value2}"}).ignore();
        m_props->setValue("prop3", std::string{"$test1{value1},$test1{value2},$test2{value1},$test2{value2}"}).ignore();

        const auto str1 = *m_props->getValue<std::string>("prop1/field1");
        EXPECT_EQ(str1, "AAA,$test1{value2}");

        const auto str2 = *m_props->getValue<std::string>("prop2/field1");
        EXPECT_EQ(str2, "$test2{value1},BBB");

        const auto str3 = *m_props->getValue<std::string>("prop3");
        EXPECT_EQ(str3, "AAA,$test1{value2},$test2{value1},BBB");
    }
}  // namespace nau::test
