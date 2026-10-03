// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <list>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "nau/serialization/runtime_value_builder.h"
#include "nau/string/string_utils.h"
#include "nau/utils/scope_guard.h"
#include "nau/utils/type_utility.h"

using namespace testing;

namespace nau::test
{
    namespace
    {
        struct StructNoFields
        {
            unsigned value;
        };

        struct OneFieldStruct1
        {
            int field = 77;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(field))
        };

        struct FooObject1
        {
            int field1 = 1;
            std::vector<unsigned> fieldArr;
            OneFieldStruct1 fieldObj;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(field1),
                CLASS_FIELD(fieldArr),
                CLASS_FIELD(fieldObj))
        };

        template <std::integral T>
        struct IntAsString
        {
            T value{};

            IntAsString() = default;

            IntAsString(T v) :
                value{v}
            {
            }

            IntAsString(const IntAsString&) = default;

        private:
            friend std::string toString(const IntAsString<T>& v)
            {
                return nau::strings::lexicalCast<T>(v.value);
            }

            friend bool parse(IntAsString<T>& v, std::string_view str)
            {
                v.value = nau::strings::lexicalCast<T>(str);
                return true;
            }
        };

        template <bool IsMutable>
        struct RuntimeValueByRef : std::bool_constant<IsMutable>
        {
            template <typename T>
            auto operator()(T& value) const
            {
                return makeValueRef(value);
            }

            bool checkMutability(const nau::RuntimeValue& value) const
            {
                return value.isMutable() == IsMutable;
            }
        };

        template <bool IsMutable>
        struct RuntimeValueCopy : std::bool_constant<IsMutable>
        {
            template <typename T>
            auto operator()(T&& value) const
            {
                return makeValueCopy(std::forward<T>(value));
            }

            bool checkMutability(const nau::RuntimeValue& value) const
            {
                return value.isMutable();
            }
        };

        struct TypeWithInfo1
        {
            NAU_TYPEID(nau::test::TypeWithInfo1)
        };

        struct TypeWithInfo2
        {
            NAU_TYPEID(nau::test::TypeWithInfo2)
        };

        template <typename Factory, typename T>
        using DeclValue = std::conditional_t<Factory::value, T, std::add_const_t<T>>;
    }  // namespace

    template <typename Factory>
    class TestMakeRuntimeValue : public testing::Test
    {
    protected:
        static constexpr bool IsMutable = Factory::value;

        template <typename NativeType>
        testing::AssertionResult checkRuntimeIntegerValue(NativeType val) const
        {
            DeclValue<Factory, NativeType> nativeValue = val;
            auto rtValue = m_valueFactory(nativeValue);
            static_assert(std::is_assignable_v<nau::RuntimeIntegerValue&, typename decltype(rtValue)::type&>);

            if (!m_valueFactory.checkMutability(*rtValue))
            {
                return testing::AssertionFailure() << "Invalid runtime value mutability";
            }

            if (rtValue->isSigned() != std::is_signed_v<NativeType>)
            {
                return testing::AssertionFailure() << "Invalid runtime value sign: " << testing::internal::GetTypeName<NativeType>();
            }

            if (rtValue->getBitsCount() != sizeof(NativeType))
            {
                return testing::AssertionFailure() << "Invalid runtime value bits:" << testing::internal::GetTypeName<NativeType>();
            }

            if (rtValue->getInt64() != static_cast<int64_t>(val))
            {
                return testing::AssertionFailure() << "Get returns unexpected integer value";
            }

            return testing::AssertionSuccess();
        }

        template <typename NativeType>
        testing::AssertionResult checkRuntimeFloatValue(NativeType val) const
        {
            DeclValue<Factory, NativeType> nativeValue = val;
            auto rtValue = m_valueFactory(nativeValue);
            static_assert(std::is_assignable_v<nau::RuntimeFloatValue&, typename decltype(rtValue)::type&>);

            if (!m_valueFactory.checkMutability(*rtValue))
            {
                return testing::AssertionFailure() << "Invalid runtime value mutability";
            }

            if (rtValue->getBitsCount() != sizeof(NativeType))
            {
                return testing::AssertionFailure() << "Invalid runtime value bits:" << testing::internal::GetTypeName<NativeType>();
            }

            return testing::AssertionSuccess();
        }

        const Factory m_valueFactory{};
    };

    using Factories = testing::Types<RuntimeValueByRef<true>, RuntimeValueByRef<false>, RuntimeValueCopy<true>, RuntimeValueCopy<false>>;
    TYPED_TEST_SUITE(TestMakeRuntimeValue, Factories);

    TYPED_TEST(TestMakeRuntimeValue, IntegerValue)
    {
        ASSERT_TRUE(this->checkRuntimeIntegerValue((char)0));
        ASSERT_TRUE(this->checkRuntimeIntegerValue((unsigned char)0));
        ASSERT_TRUE(this->checkRuntimeIntegerValue(static_cast<int16_t>(0)));
        ASSERT_TRUE(this->checkRuntimeIntegerValue(static_cast<uint16_t>(0)));
        ASSERT_TRUE(this->checkRuntimeIntegerValue(static_cast<int32_t>(0)));
        ASSERT_TRUE(this->checkRuntimeIntegerValue(static_cast<uint32_t>(0)));
        ASSERT_TRUE(this->checkRuntimeIntegerValue(static_cast<int64_t>(0)));
        ASSERT_TRUE(this->checkRuntimeIntegerValue(static_cast<uint64_t>(0)));
    }

    TYPED_TEST(TestMakeRuntimeValue, BooleanValue)
    {
        DeclValue<TypeParam, bool> value = true;
        auto rtValue = this->m_valueFactory(value);
        static_assert(std::is_same_v<nau::RuntimeBooleanValue::Ptr, decltype(rtValue)>);

        ASSERT_TRUE(this->m_valueFactory.checkMutability(*rtValue));
        ASSERT_THAT(rtValue->getBool(), Eq(value));
    }

    TYPED_TEST(TestMakeRuntimeValue, FloatPointValue)
    {
        ASSERT_TRUE(this->checkRuntimeFloatValue(0.f));
        ASSERT_TRUE(this->checkRuntimeFloatValue(0.0));
    }

    TYPED_TEST(TestMakeRuntimeValue, StringValue)
    {
        DeclValue<TypeParam, std::string> str;
        auto rtValue1 = this->m_valueFactory(str);

        ASSERT_TRUE(rtValue1->template is<RuntimeStringValue>());

        // std::string_view strView;
        // auto rtValue2 = _valueFactory(strView);
    }

    TYPED_TEST(TestMakeRuntimeValue, EastlStringValue)
    {
        DeclValue<TypeParam, std::string> str;
        auto rtValue1 = this->m_valueFactory(str);

        ASSERT_TRUE(rtValue1->template is<RuntimeStringValue>());

        // std::string_view strView;
        // auto rtValue2 = _valueFactory(strView);
    }

    TYPED_TEST(TestMakeRuntimeValue, StringSerializable)
    {
        /*DeclValue<TypeParam, IntAsString<uint64_t>> iValue {78945};

        static_assert(nau::IsStringSerializable<IntAsString<uint64_t>>);

        auto rtVal = this->_valueFactory(iValue);*/
        /*
        ASSERT_TRUE(rtVal->Is<nau::RuntimeStringValue>());
        ASSERT_THAT(rtVal->GetUtf8(), Eq("78945"));

        if constexpr (IsMutable) {
            rtVal->SetUtf8("1234");
            ASSERT_THAT(iValue.value, Eq(1234));
        }*/
    }

    TYPED_TEST(TestMakeRuntimeValue, OptionalValue)
    {
        DeclValue<TypeParam, std::optional<unsigned>> opt;
        auto value = this->m_valueFactory(opt);

        static_assert(std::is_assignable_v<nau::RuntimeOptionalValue&, typename decltype(value)::type&>);
    }

    TEST(TestRuntimeValue, CollectionValue)
    {
        {
            std::vector<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_TRUE(value->isMutable());
        }

        {
            const std::vector<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }

        {
            std::list<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_TRUE(value->isMutable());
        }

        {
            const std::list<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }

        {
            const std::set<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }

        {
            const std::unordered_set<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }
    }

    TEST(TestRuntimeValue, CollectionValue_Eastl)
    {
        {
            std::vector<int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
            ASSERT_TRUE(value->isMutable());
        }
        {
            std::list<int> arr;
            constexpr bool IsList = LikeStdList<decltype(arr)>;
            static_assert(IsList);

            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
        }

        {
            std::set<int> arr;
            static_assert(LikeSet<decltype(arr)>);

            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
        }

        {
            std::unordered_set<int> arr;
            static_assert(LikeSet<decltype(arr)>);

            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeCollection&, typename decltype(value)::type&>);
        }
    }

    TYPED_TEST(TestMakeRuntimeValue, TupleValue)
    {
        DeclValue<TypeParam, std::tuple<int, float>> tuple1 = {};

        auto rtValue = this->m_valueFactory(tuple1);
        static_assert(std::is_assignable_v<nau::RuntimeReadonlyCollection&, typename decltype(rtValue)::type&>);

        ASSERT_TRUE(this->m_valueFactory.checkMutability(*rtValue));
        ASSERT_THAT(rtValue->getSize(), Eq(std::tuple_size_v<decltype(tuple1)>));
    }

    TEST(TestRuntimeValue, DictionaryValue)
    {
        {
            std::map<std::string, int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeDictionary&, decltype(value)::type&>);
            ASSERT_TRUE(value->isMutable());
        }

        {
            const std::map<std::string, int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeDictionary&, decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }
    }

    TEST(TestRuntimeValue, DictionaryValue_Eastl)
    {
        {
            std::map<std::string, int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeDictionary&, decltype(value)::type&>);
            ASSERT_TRUE(value->isMutable());
        }

        {
            const std::map<std::string, int> arr;
            auto value = nau::makeValueRef(arr);
            static_assert(std::is_assignable_v<nau::RuntimeDictionary&, decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }
    }

    TEST(TestRuntimeValue, ObjectValue)
    {
        {
            FooObject1 obj;
            auto value = nau::makeValueRef(obj);
            static_assert(std::is_assignable_v<nau::RuntimeObject&, decltype(value)::type&>);
            ASSERT_TRUE(value->isMutable());
        }
        {
            const FooObject1 obj;
            auto value = nau::makeValueRef(obj);
            static_assert(std::is_assignable_v<nau::RuntimeObject&, decltype(value)::type&>);
            ASSERT_FALSE(value->isMutable());
        }
    }

    TEST(TestRuntimeValue, ObjectValue_FieldAccess)
    {
        constexpr unsigned ExpectedValue = 77;

        FooObject1 obj;
        auto runtimeObj = nau::makeValueRef(obj);

        auto fieldValue = runtimeObj->getValue("field1");
        fieldValue->as<RuntimeIntegerValue&>().set(ExpectedValue);

        ASSERT_EQ(obj.field1, ExpectedValue);
    }

    TEST(TestRuntimeValue, ObjectValue_FieldAccessParentDead)
    {
        constexpr unsigned ExpectedValue = 77;

        FooObject1 obj;

        RuntimeIntegerValue::Ptr fieldValue = nau::makeValueRef(obj)->getValue("field1");
        fieldValue->set(ExpectedValue);
        ASSERT_EQ(obj.field1, ExpectedValue);
    }

    TEST(TestRuntimeValue, RuntimeValueRef)
    {
        FooObject1 obj;
        auto value = nau::makeValueRef(obj);

        // auto valueRef = nau::makeValueRef(value);
        // static_assert(std::is_same_v<decltype(valueRef), decltype(value)>);

        // auto valueCopy = nau::makeValueCopy(value);
        // static_assert(std::is_same_v<decltype(valueCopy), decltype(value)>);
    }

    TEST(TestRuntimeValue, StringSerializable)
    {
    }

    TEST(TestRuntimeValue, HasRepresentation)
    {
        static_assert(HasRuntimeValueRepresentation<char>);
        static_assert(HasRuntimeValueRepresentation<short>);
        static_assert(HasRuntimeValueRepresentation<int>);
        static_assert(HasRuntimeValueRepresentation<unsigned>);
        static_assert(HasRuntimeValueRepresentation<float>);
        static_assert(HasRuntimeValueRepresentation<double>);

        static_assert(HasRuntimeValueRepresentation<OneFieldStruct1>);

        static_assert(HasRuntimeValueRepresentation<std::optional<OneFieldStruct1>>);
        static_assert(HasRuntimeValueRepresentation<std::optional<unsigned>>);

        static_assert(HasRuntimeValueRepresentation<std::optional<std::vector<OneFieldStruct1>>>);

        static_assert(HasRuntimeValueRepresentation<std::vector<OneFieldStruct1>>);
        static_assert(HasRuntimeValueRepresentation<std::map<std::string, OneFieldStruct1>>);

        static_assert(HasRuntimeValueRepresentation<std::tuple<OneFieldStruct1, float>>);

        static_assert(HasRuntimeValueRepresentation<std::set<unsigned, float>>);
        static_assert(HasRuntimeValueRepresentation<std::unordered_set<unsigned, float>>);
        static_assert(HasRuntimeValueRepresentation<std::set<unsigned, float>>);
        static_assert(HasRuntimeValueRepresentation<std::unordered_set<unsigned, float>>);
    }

    TEST(TestRuntimeValue, HasNoRepresentation)
    {
        static_assert(!HasRuntimeValueRepresentation<StructNoFields>);
        static_assert(!HasRuntimeValueRepresentation<std::optional<StructNoFields>>);
        static_assert(!HasRuntimeValueRepresentation<std::tuple<float, StructNoFields>>);
    }

    /**
        Test: assign primitive field value through dynamic properties map (i.e. object = map<field_name, RuntimeValue::Ptr>)
     */
    TEST(TestRuntimeValue, AssignWrappedPrimitiveValue)
    {
        auto runtimeValueProps = EXPR_Block->RuntimeValue::Ptr
        {
            std::unordered_map<std::string, RuntimeValue::Ptr> properties;
            properties["field1"] = makeValueCopy(77);

            return makeValueCopy(std::move(properties));
        };

        FooObject1 targetObject;
        Result<> assignResult = RuntimeValue::assign(makeValueRef(targetObject), runtimeValueProps);
        ASSERT_TRUE(assignResult);
        ASSERT_EQ(targetObject.field1, 77);
    }

    /**
        Test: assign collection field value through dynamic properties map (i.e. object = map<field_name, RuntimeValue::Ptr>)
     */
    TEST(TestRuntimeValue, AssignWrappedCollectionValue)
    {
        auto runtimeValueProps = EXPR_Block->RuntimeValue::Ptr
        {
            std::unordered_map<std::string, RuntimeValue::Ptr> properties;

            properties["fieldArr"] = makeValueCopy(std::vector<unsigned>{100, 200});
            return makeValueCopy(std::move(properties));
        };

        FooObject1 targetObject;
        Result<> assignResult = RuntimeValue::assign(makeValueRef(targetObject), runtimeValueProps);
        ASSERT_TRUE(assignResult);
        ASSERT_EQ(targetObject.fieldArr.size(), 2);
        ASSERT_EQ(targetObject.fieldArr[0], 100);
        ASSERT_EQ(targetObject.fieldArr[1], 200);
    }

    /**
        Test: assign object field value through dynamic properties map (i.e. object = map<field_name, RuntimeValue::Ptr>)
     */
    TEST(TestRuntimeValue, AssignWrappedObjectValue)
    {
        auto runtimeValueProps = EXPR_Block->RuntimeValue::Ptr
        {
            std::unordered_map<std::string, RuntimeValue::Ptr> objectProperties;
            objectProperties["field"] = makeValueCopy(99);

            std::unordered_map<std::string, RuntimeValue::Ptr> properties;
            properties["fieldObj"] = makeValueCopy(std::move(objectProperties));
            return makeValueCopy(std::move(properties));
        };

        FooObject1 targetObject;
        Result<> assignResult = RuntimeValue::assign(makeValueRef(targetObject), runtimeValueProps);
        ASSERT_TRUE(assignResult);
        ASSERT_EQ(targetObject.fieldObj.field, 99);
    }

    /**
        Test: assign dictionary value through dynamic properties map (i.e. object = map<field_name, RuntimeValue::Ptr>)
     */
    TEST(TestRuntimeValue, AssignWrappedDictionaryValue)
    {
        auto runtimeValueProps = EXPR_Block->RuntimeValue::Ptr
        {
            std::unordered_map<std::string, RuntimeValue::Ptr> properties;
            properties["key1"] = makeValueCopy(77);
            properties["key2"] = makeValueCopy(88.f);
            properties["key3"] = makeValueCopy(std::optional<unsigned>{99});
            return makeValueCopy(std::move(properties));
        };

        std::map<std::string, unsigned> targetObject;
        Result<> assignResult = RuntimeValue::assign(makeValueRef(targetObject), runtimeValueProps);
        ASSERT_TRUE(assignResult);
        ASSERT_EQ(targetObject.size(), 3);
        ASSERT_EQ(targetObject["key1"], 77);
        ASSERT_EQ(targetObject["key2"], 88);
        ASSERT_EQ(targetObject["key3"], 99);
    }

    /**
     */
    TEST(TestRuntimeValue, KnownSetCollections)
    {
        static_assert(LikeSet<std::set<unsigned, std::less<>>>);
        static_assert(LikeSet<std::unordered_set<unsigned>>);
        static_assert(LikeSet<std::set<unsigned>>);
        static_assert(LikeSet<std::unordered_set<unsigned>>);
    }

    namespace
    {
        inline std::vector<unsigned> makeSortedCollection(RuntimeCollection& inCollection)
        {
            std::vector<unsigned> sortedCollection;
            sortedCollection.reserve(inCollection.getSize());

            for (size_t i = 0, size = inCollection.getSize(); i < size; ++i)
            {
                const auto val = *runtimeValueCast<unsigned>(inCollection[i]);
                sortedCollection.push_back(val);
            }

            std::sort(sortedCollection.begin(), sortedCollection.end());
            return sortedCollection;
        }
    }  // namespace

    /**
        Test: std::set collection
    */

    TEST(TestRuntimeValue, EastlSet)
    {
        std::set<unsigned> values = {2, 1, 4, 3, 5, 7, 6, 0};
        {
            auto runtimeCollection = makeValueRef(values);
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            for (size_t i = 0; i < runtimeCollection->getSize(); ++i)
            {
                const auto val = *runtimeValueCast<unsigned>(runtimeCollection->getAt(i));
                ASSERT_EQ(val, i);
            }
        }

        {
            auto runtimeCollection = makeValueCopy(std::move(values));
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            for (size_t i = 0; i < runtimeCollection->getSize(); ++i)
            {
                const auto val = *runtimeValueCast<unsigned>(runtimeCollection->getAt(i));
                ASSERT_EQ(val, i);
            }
        }
    }

    /**
        Test: std::unordered_set collection
    */
    TEST(TestRuntimeValue, EastlUnorderedSet)
    {
        std::unordered_set<unsigned> values = {2, 1, 4, 3, 5, 7, 6, 0};
        {
            auto runtimeCollection = makeValueRef(values);
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            ASSERT_TRUE(runtimeCollection->isMutable());

            auto values = makeSortedCollection(*runtimeCollection);
            for (size_t i = 0; i < values.size(); ++i)
            {
                ASSERT_EQ(values[i], i);
            }
        }

        {
            auto runtimeCollection = makeValueCopy(std::move(values));
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            ASSERT_TRUE(runtimeCollection->isMutable());

            auto values = makeSortedCollection(*runtimeCollection);
            for (size_t i = 0; i < values.size(); ++i)
            {
                ASSERT_EQ(values[i], i);
            }
        }
    }

    /**
        Test: std::set collection
    */
    TEST(TestRuntimeValue, StdSet)
    {
        std::set<unsigned> values = {2, 1, 4, 3, 5, 7, 6, 0};
        {
            auto runtimeCollection = makeValueRef(const_cast<const decltype(values)&>(values));
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            ASSERT_FALSE(runtimeCollection->isMutable());
            for (size_t i = 0; i < runtimeCollection->getSize(); ++i)
            {
                const auto val = *runtimeValueCast<unsigned>(runtimeCollection->getAt(i));
                ASSERT_EQ(val, i);
            }
        }

        {
            auto runtimeCollection = makeValueCopy(std::move(values));
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            for (size_t i = 0; i < runtimeCollection->getSize(); ++i)
            {
                const auto val = *runtimeValueCast<unsigned>(runtimeCollection->getAt(i));
                ASSERT_EQ(val, i);
            }
        }
    }

    /**
        Test: std::unordered_set collection
    */
    TEST(TestRuntimeValue, StdUnorderedSet)
    {
        std::unordered_set<unsigned> values = {2, 1, 4, 3, 5, 7, 6, 0};
        {
            auto runtimeCollection = makeValueRef(values);
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            ASSERT_TRUE(runtimeCollection->isMutable());

            auto values = makeSortedCollection(*runtimeCollection);
            for (size_t i = 0; i < values.size(); ++i)
            {
                ASSERT_EQ(values[i], i);
            }
        }

        {
            auto runtimeCollection = makeValueCopy(std::move(values));
            ASSERT_EQ(runtimeCollection->getSize(), 8);
            ASSERT_TRUE(runtimeCollection->isMutable());

            auto values = makeSortedCollection(*runtimeCollection);
            for (size_t i = 0; i < values.size(); ++i)
            {
                ASSERT_EQ(values[i], i);
            }
        }
    }

    /**
     */
    TEST(TestRuntimeValue, TypeInfoBasic)
    {
        auto runtimeTypeInfoValue = makeValueCopy(rtti::getTypeInfo<TypeWithInfo1>());
        ASSERT_TRUE(runtimeTypeInfoValue);
        ASSERT_TRUE(runtimeTypeInfoValue->is<RuntimeIntegerValue>());

        const size_t typeId = *runtimeValueCast<size_t>(runtimeTypeInfoValue);
        ASSERT_EQ(rtti::getTypeInfo<TypeWithInfo1>(), rtti::makeTypeInfoFromId(typeId));
    }

    /**
     */
    TEST(TestRuntimeValue, TypeInfCollection)
    {
        auto collection = makeValueCopy(rtti::makeTypeInfoCollection<TypeWithInfo1, TypeWithInfo2>());
        ASSERT_TRUE(collection);

        auto types = *runtimeValueCast<std::vector<rtti::TypeInfo>>(collection);

        ASSERT_EQ(types.size(), 2);
        ASSERT_EQ(types[0], rtti::getTypeInfo<TypeWithInfo1>());
        ASSERT_EQ(types[1], rtti::getTypeInfo<TypeWithInfo2>());
    }

}  // namespace nau::test
