// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_runtime_value.cpp

#include <map>
#include <vector>

#include "nau/serialization/runtime_value_builder.h"
#include "nau/serialization/runtime_value_events.h"

namespace nau::test
{
    namespace
    {
        struct FooObject
        {
            int field1 = 1;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(field1))
        };
    }  // namespace

    class TestRuntimeValueEvents : public ::testing::Test
    {
    protected:
        void subscribeOnChanges(RuntimeValue& value)
        {
            m_subscription = value.as<IRuntimeValueEvents&>().subscribeOnChanges([this](const RuntimeValue&, std::string_view)
            {
                m_isChanged = true;
            });
        }

        template <typename T>
        requires(nau::HasRuntimeValueRepresentation<T>)
        auto makeValueRefAndSubscribe(T& value)
        {
            auto runtimeValue = makeValueRef(value);
            runtimeValue->template as<IRuntimeValueEvents&>();
            subscribeOnChanges(*runtimeValue);

            return runtimeValue;
        }

        bool isChanged() const
        {
            return m_isChanged;
        }

        void resetChanged()
        {
            m_isChanged = false;
        }

        void resetSubscription()
        {
            m_subscription = nullptr;
        }

    private:
        IRuntimeValueEvents::SubscriptionHandle m_subscription;
        bool m_isChanged = false;
    };

    /**
     */
    TEST_F(TestRuntimeValueEvents, ValuesHasEventsApi)
    {
        ASSERT_TRUE(makeValueCopy(unsigned{77})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(float{77.f})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(bool{true})->is<IRuntimeValueEvents>());
        // ASSERT_TRUE(makeValueCopy(std::string{})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(std::optional<float>{77.f})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(std::vector<float>{})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(std::tuple<float, unsigned>{})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(std::map<std::string, unsigned>{})->is<IRuntimeValueEvents>());
        ASSERT_TRUE(makeValueCopy(FooObject{})->is<IRuntimeValueEvents>());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeInteger)
    {
        unsigned value = 77;
        makeValueRefAndSubscribe(value)->set(88);
        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeBoolean)
    {
        unsigned value = false;
        makeValueRefAndSubscribe(value)->set(true);
        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeFloat)
    {
        float value = 77.f;
        makeValueRefAndSubscribe(value)->set(88.f);
        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeOptional)
    {
        std::optional<float> value = std::nullopt;
        const RuntimeOptionalValue::Ptr runtimeValue = makeValueRefAndSubscribe(value);
        runtimeValue->setValue(makeValueCopy(77)).ignore();
        ASSERT_TRUE(isChanged());

        resetChanged();
        ASSERT_FALSE(isChanged());

        runtimeValue->getValue()->as<RuntimeFloatValue&>().set(99.f);
        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeCollection_Append)
    {
        std::vector<float> collection;
        const RuntimeCollection::Ptr runtimeValue = makeValueRefAndSubscribe(collection);
        runtimeValue->append(makeValueCopy(1.f)).ignore();
        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeCollection_Clear)
    {
        std::vector<float> collection;
        const RuntimeCollection::Ptr runtimeValue = makeValueRefAndSubscribe(collection);
        runtimeValue->append(makeValueCopy(1.f)).ignore();

        resetChanged();
        ASSERT_FALSE(isChanged());
        runtimeValue->clear();
        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeCollection_ChangeElement_1)
    {
        std::vector<float> collection;
        const RuntimeCollection::Ptr runtimeValue = makeValueRefAndSubscribe(collection);
        runtimeValue->append(makeValueCopy(1.f)).ignore();
        runtimeValue->append(makeValueCopy(2.f)).ignore();

        resetChanged();
        ASSERT_FALSE(isChanged());

        {
            auto element = runtimeValue->getAt(1);
            element->as<RuntimeFloatValue&>().set(22.f);
        }

        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeCollection_ChangeElement_2)
    {
        std::vector<FooObject> collection;
        const RuntimeCollection::Ptr runtimeValue = makeValueRefAndSubscribe(collection);
        runtimeValue->append(makeValueCopy(FooObject{1})).ignore();
        runtimeValue->append(makeValueCopy(FooObject{2})).ignore();

        resetChanged();
        ASSERT_FALSE(isChanged());

        {
            auto element = (*runtimeValue)[1];
            auto field = element->as<RuntimeObject&>()["field1"];

            // resetting element.
            // but change notifications must still propagate up to the top parent object
            element.reset();
            field->as<RuntimeIntegerValue&>().set(22);
        }

        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, ChangeDictionary_Set)
    {
        std::map<std::string, FooObject> dict;

        const RuntimeDictionary::Ptr runtimeValue = makeValueRefAndSubscribe(dict);
        runtimeValue->setValue("one", makeValueCopy(FooObject{1})).ignore();
        runtimeValue->setValue("two", makeValueCopy(FooObject{1})).ignore();

        ASSERT_TRUE(isChanged());
    }

    TEST_F(TestRuntimeValueEvents, ChangeDictionary_ChangeElement)
    {
        std::map<std::string, FooObject> dict;
        const RuntimeDictionary::Ptr runtimeValue = makeValueRefAndSubscribe(dict);
        runtimeValue->setValue("one", makeValueCopy(FooObject{1})).ignore();
        runtimeValue->setValue("two", makeValueCopy(FooObject{1})).ignore();

        resetChanged();
        ASSERT_FALSE(isChanged());

        {
            auto element = (*runtimeValue)["one"];
            auto field = element->as<RuntimeObject&>()["field1"];
            element.reset();
            field->as<RuntimeIntegerValue&>().set(22);
        }

        ASSERT_TRUE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, Unsubscribe)
    {
        unsigned value = 77;
        RuntimeIntegerValue::Ptr runtimeValue = makeValueRefAndSubscribe(value);
        resetSubscription();

        runtimeValue->set(88);
        ASSERT_FALSE(isChanged());
    }

    /**
     */
    TEST_F(TestRuntimeValueEvents, UnsubscribeAfterObjectIsDead)
    {
        unsigned value = 77;
        RuntimeIntegerValue::Ptr runtimeValue = makeValueRefAndSubscribe(value);

        runtimeValue.reset();
        resetSubscription();
    }
}  // namespace nau::test
