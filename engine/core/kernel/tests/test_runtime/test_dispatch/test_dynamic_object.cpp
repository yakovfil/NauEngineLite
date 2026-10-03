// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <span>

#include "nau/dispatch/class_descriptor_builder.h"
#include "nau/dispatch/dynamic_object_impl.h"
#include "nau/serialization/runtime_value_events.h"

namespace nau::test
{
    using namespace ::testing;

    namespace
    {
        NAU_DEFINE_ATTRIBUTE(Attrib0, "test.attrib_0", meta::AttributeOptionsNone)
        NAU_DEFINE_ATTRIBUTE(Attrib1, "test.attrib_1", meta::AttributeOptionsNone)

        struct ITestInterface1
        {
            NAU_TYPEID(ITestInterface1)

            NAU_CLASS_ATTRIBUTES(
                CLASS_NAME_ATTRIBUTE("TestInterface1"))
        };

        class FooClass1 : public DynamicObjectImpl,
                          public virtual ITestInterface1
        {
            NAU_CLASS_(FooClass1, DynamicObjectImpl, ITestInterface1)

            NAU_CLASS_FIELDS(
                CLASS_FIELD(m_text),
                CLASS_FIELD(m_value1))

            NAU_CLASS_METHODS(
                CLASS_METHOD(FooClass1, getText),
                CLASS_METHOD(FooClass1, getValue1),
                CLASS_METHOD(FooClass1, setText),
                CLASS_METHOD(FooClass1, setValue1))

            NAU_CLASS_ATTRIBUTES(
                CLASS_ATTRIBUTE(Attrib0, 11),
                CLASS_ATTRIBUTE(Attrib1, std::string{"Second"}),
                CLASS_NAME_ATTRIBUTE("FooClass1"))

            NAU_DECLARE_DYNAMIC_OBJECT
        public:
            FooClass1()
            {
            }

            const std::string& getText() const
            {
                return m_text;
            }

            unsigned getValue1() const
            {
                return m_value1;
            }

            void setText(std::string_view text)
            {
                value_changes_scope;
                m_text = text;
            }

            void setValue1(unsigned value)
            {
                value_changes_scope;
                m_value1 = value;
            }

        private:
            std::string m_text{"initial_text"};
            unsigned m_value1 = 11;
        };

        NAU_IMPLEMENT_DYNAMIC_OBJECT(FooClass1)

        class FooClass2 : public FooClass1
        {
            NAU_CLASS_(nau::test::FooClass2, FooClass1)

            NAU_CLASS_FIELDS(
                CLASS_NAMED_FIELD(m_value2, "value2"))

            NAU_CLASS_METHODS(
                CLASS_METHOD(FooClass2, getValue2),
                CLASS_METHOD(FooClass2, setValue2));

            NAU_DECLARE_DYNAMIC_OBJECT

        public:
            FooClass2()
            {
            }

        private:
            unsigned getValue2()
            {
                return m_value2;
            }

            void setValue2(unsigned value)
            {
                value_changes_scope;
                m_value2 = value;
            }

            unsigned m_value2 = 22;
        };

        NAU_IMPLEMENT_DYNAMIC_OBJECT(FooClass2)

    }  // namespace

    /**
     */
    TEST(TestDynamicObject, ContainsFields)
    {
        DynamicObject::Ptr obj = rtti::createInstance<FooClass2>();

        ASSERT_TRUE(obj->containsKey("m_text"));
        ASSERT_TRUE(obj->containsKey("m_value1"));
        ASSERT_TRUE(obj->containsKey("value2"));
    }

    /**
     */
    TEST(TestDynamicObject, CheckClassDescriptor)
    {
        DynamicObject::Ptr obj = rtti::createInstance<FooClass2>();

        const IClassDescriptor::Ptr classDesc = obj->getClassDescriptor();

        ASSERT_TRUE(classDesc->hasInterface<FooClass1>());
        ASSERT_TRUE(classDesc->hasInterface<FooClass2>());

        ASSERT_NE(classDesc->findMethod("getText"), nullptr);
        ASSERT_NE(classDesc->findMethod("getValue1"), nullptr);
        ASSERT_NE(classDesc->findMethod("setText"), nullptr);
        ASSERT_NE(classDesc->findMethod("setValue1"), nullptr);
        ASSERT_NE(classDesc->findMethod("getValue2"), nullptr);
        ASSERT_NE(classDesc->findMethod("setValue2"), nullptr);
    }

    /**
     */
    TEST(TestDynamicObject, AccessField)
    {
        constexpr unsigned ExpectedValue = 77;

        DynamicObject::Ptr obj = rtti::createInstance<FooClass2>();
        obj->setFieldValue("m_value1", makeValueCopy(ExpectedValue)).ignore();

        const auto classDesc = obj->getClassDescriptor();
        const IMethodInfo* const method = classDesc->findMethod("getValue1");
        const RuntimeIntegerValue::Ptr result = method->invokeToPtr(obj.get(), {});

        ASSERT_THAT(result, NotNull());
        ASSERT_EQ(*runtimeValueCast<unsigned>(result), ExpectedValue);
    }

    /**
     */
    TEST(TestDynamicObject, TrackChanges)
    {
        constexpr unsigned ExpectedValue = 77;

        DynamicObject::Ptr obj = rtti::createInstance<FooClass2>();

        unsigned changesCounter = 0;
        auto subscription = obj->as<IRuntimeValueEvents&>().subscribeOnChanges([&changesCounter](const RuntimeValue&, std::string_view)
        {
            ++changesCounter;
        });

        obj->setFieldValue("m_value1", makeValueCopy(ExpectedValue)).ignore();
        ASSERT_EQ(changesCounter, 1);

        const auto classDesc = obj->getClassDescriptor();
        const IMethodInfo* const method = classDesc->findMethod("setValue2");
        method->invoke(obj.get(), {makeValueCopy(99)}).ignore();

        ASSERT_EQ(changesCounter, 2);
    }

    /**
     */
    TEST(TestDynamicObject, GetInterface)
    {
        auto descriptor = rtti::createInstance<nau_detail::ClassDescriptorImpl<FooClass1>>();

        const IInterfaceInfo* const itf = descriptor->findInterface<ITestInterface1>();
        ASSERT_TRUE(itf);
        ASSERT_EQ(itf->getName(), "TestInterface1");
    }

    /**
     */
    TEST(TestDynamicObject, Attributes)
    {
        auto descriptor = rtti::createInstance<nau_detail::ClassDescriptorImpl<FooClass1>>();

        const auto* attributes = descriptor->getClassAttributes();
        ASSERT_TRUE(attributes);

        attributes->containsAttribute(Attrib0{}.strValue);
        attributes->containsAttribute(Attrib1{}.strValue);
    }

    /**
     */
    TEST(TestDynamicObject, EmptyAttributes)
    {
        auto descriptor = rtti::createInstance<nau_detail::ClassDescriptorImpl<FooClass2>>();

        const auto* attributes = descriptor->getClassAttributes();
        ASSERT_TRUE(attributes);
        ASSERT_EQ(attributes->getSize(), 0);
    }

    /**
     */
    TEST(TestDynamicObject, ClassName)
    {
        auto descriptor = rtti::createInstance<nau_detail::ClassDescriptorImpl<FooClass1>>();
        ASSERT_EQ(descriptor->getClassName(), "FooClass1");
    }

    /**
     */
    TEST(TestDynamicObject, ClassDefaultName)
    {
        auto descriptor = rtti::createInstance<nau_detail::ClassDescriptorImpl<FooClass2>>();
        ASSERT_EQ(descriptor->getClassName(), "nau::test::FooClass2");
    }
}  // namespace nau::test
