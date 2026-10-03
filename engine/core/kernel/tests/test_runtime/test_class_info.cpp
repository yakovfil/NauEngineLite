// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/meta/class_info.h"

using namespace testing;

namespace nau::test
{
    namespace
    {
        struct TypeWithNoInfo
        {
        };

        struct Base0
        {
        };

        struct Base1 : virtual Base0
        {
            NAU_CLASS_BASE(Base0)
        };

        struct Base2
        {
        };

        struct Base3 : Base2,
                       virtual Base0
        {
            NAU_CLASS_BASE(Base2, Base0)
        };

        struct MyClass : Base1,
                         Base3
        {
            NAU_CLASS_BASE(Base1, Base3)
        };

        struct TypeWithFields
        {
            static constexpr const char* DefaultStr = "default";
            static constexpr int DefaultInt = 77;

            int intField = DefaultInt;
            std::string strField = DefaultStr;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(intField),
                CLASS_FIELD(strField))
        };

        class TypeWithNamedFields
        {
        public:
            static constexpr int DefaultInt1 = 11;
            static constexpr int DefaultInt2 = 22;

            NAU_CLASS_FIELDS(
                CLASS_NAMED_FIELD(m_field1, "field1"),
                CLASS_NAMED_FIELD(m_field2, "field2"));

            int m_field1 = DefaultInt1;
            int m_field2 = DefaultInt2;
        };

        class InheritFields1 : public TypeWithNamedFields
        {
            NAU_CLASS_BASE(TypeWithNamedFields)
        };

        class InheritFields2 : public InheritFields1
        {
            NAU_CLASS_BASE(InheritFields1)

            NAU_CLASS_FIELDS(
                CLASS_FIELD(m_field3))

            unsigned m_field3;
        };

        class TypeCompoundFileds : public TypeWithFields,
                                   public TypeWithNamedFields
        {
        public:
            NAU_CLASS_BASE(TypeWithFields, TypeWithNamedFields)

            NAU_CLASS_FIELDS(
                CLASS_FIELD(m_field3),
                CLASS_FIELD(m_field4))

            int m_field3 = DefaultInt1;
            float m_field4 = DefaultInt2;
        };

        class TypeWithMethods
        {
            NAU_CLASS_METHODS(
                CLASS_METHOD(TypeWithMethods, method1),
                CLASS_METHOD(TypeWithMethods, method2))
        public:
            void method1()
            {
            }

            unsigned method2(float, float) const
            {
                return 0;
            }
        };

        class TypeWithNamedMethods
        {
            NAU_CLASS_METHODS(
                CLASS_NAMED_METHOD(TypeWithNamedMethods, method3, "methodThird"),
                CLASS_NAMED_METHOD(TypeWithNamedMethods, method4, "methodFourth"))

        public:
            void method3()
            {
            }

            unsigned method4(float, float) const
            {
                return 0;
            }
        };

        class TypeCompoundMethods : public TypeWithMethods,
                                    public TypeWithNamedMethods
        {
            NAU_CLASS_BASE(TypeWithMethods, TypeWithNamedMethods);
            NAU_CLASS_METHODS(
                CLASS_METHOD(TypeCompoundMethods, method5),
                CLASS_METHOD(TypeCompoundMethods, method6))

            void method5()
            {
            }

            void method6()
            {
            }
        };

        template <typename ExpectedT, typename... ExpectedAttribs, typename T, typename... Attribs>
        AssertionResult checkField(const meta::FieldInfo<T, Attribs...> field, std::string_view name, std::optional<T> value = std::nullopt)
        {
            static_assert(std::is_same_v<ExpectedT, T>, "field type mismatch");
            static_assert((std::is_same_v<ExpectedAttribs, Attribs> && ...), "field attribute type mismatch");

            if (!field.name() != name)
            {
                return AssertionFailure() << format("Field name ({0}) mismatch ({1})", field.name(), name);
            }

            return AssertionSuccess();
        }

        template <size_t... I, typename... Fields, typename... ExpectedFields>
        AssertionResult checkFieldsHelper(std::index_sequence<I...>, const std::tuple<Fields...>& fields, const std::tuple<ExpectedFields...>& expectedFields)
        {
            using namespace nau::meta;

            static_assert(sizeof...(Fields) == sizeof...(ExpectedFields), "Invalid fields count");
            static_assert((std::is_same_v<Fields, ExpectedFields> && ...), "Field value type mismatch");

            const bool success = ((std::get<I>(fields).getName() == std::get<I>(expectedFields).getName()) && ...);
            if (!success)
            {
                return AssertionFailure() << "Field name mismatch";
            }

            return AssertionSuccess();
        }

        template <typename... Fields, typename... ExpectedFields>
        AssertionResult checkFields(const std::tuple<Fields...>& fields, ExpectedFields... expected)
        {
            return checkFieldsHelper(std::make_index_sequence<sizeof...(Fields)>{}, fields, std::tuple{expected...});
        }

        template <typename... Method, typename... ExpectedMethod>
        AssertionResult checkMethods(const std::tuple<Method...>& methods, ExpectedMethod... expected)
        {
            using namespace nau::meta;

            static_assert(sizeof...(Method) == sizeof...(ExpectedMethod), "Invalid methods count");
            static_assert((std::is_same_v<std::remove_const_t<Method>, std::remove_const_t<ExpectedMethod>> && ...));

            const auto expectedMethodsTuple = std::tuple{expected...};

            constexpr auto compareMethod = []<auto Func1, auto Func2>(MethodInfo<Func1> m1, MethodInfo<Func2> m2)
            {
                static_assert(std::is_same_v<decltype(Func1), decltype(Func2)>);
                return m1.getName() == m2.getName();
            };

            const bool success = [&]<size_t... I>(std::index_sequence<I...>)
            {
                return (compareMethod(std::get<I>(methods), std::get<I>(expectedMethodsTuple)) && ...);
            }(std::make_index_sequence<sizeof...(Method)>{});

            if (!success)
            {
                return AssertionFailure() << "Method name/signature mismatch";
            }

            return AssertionSuccess();
        }

    }  // namespace


    /**
     */
    TEST(TestClassInfo, Bases)
    {
        using ActualDirectBase = meta::ClassDirectBase<MyClass>;
        static_assert(std::is_same_v<ActualDirectBase, TypeList<Base1, Base3>>);

        using ActualAllBase = meta::ClassAllBase<MyClass>;
        static_assert(std::is_same_v<ActualAllBase, TypeList<Base1, Base3, Base0, Base2, Base0>>);

        using ActualAllUniqueBase = meta::ClassAllUniqueBase<MyClass>;
        static_assert(std::is_same_v<ActualAllUniqueBase, TypeList<Base1, Base3, Base0, Base2>>);
    }

    /**
     */
    TEST(TestClassInfo, NoBases)
    {
        static_assert(std::is_same_v<meta::ClassAllBase<TypeWithNoInfo>, TypeList<>>);
    }

    /**
     */
    TEST(TestClassInfo, CheckHasFields)
    {
        static_assert(!meta::ClassHasFields<TypeWithNoInfo>);
        static_assert(meta::ClassHasFields<TypeWithFields>);
    }

    /**
     */
    TEST(TestClassInfo, GetFields)
    {
        const TypeWithFields instance;

        auto fields = meta::getClassAllFields<decltype(instance)>();

        const auto result = checkFields(fields,
                                        meta::FieldInfo{&TypeWithFields::intField, "intField"},
                                        meta::FieldInfo{&TypeWithFields::strField, "strField"});

        ASSERT_TRUE(result);
    }

    /**
    */
    TEST(TestClassInfo, NamedFields)
    {
        auto fields = meta::getClassAllFields<TypeWithNamedFields>();

        const auto result = checkFields(fields,
                                        meta::FieldInfo{&TypeWithNamedFields::m_field1, "field1"},
                                        meta::FieldInfo{&TypeWithNamedFields::m_field2, "field2"});

        ASSERT_TRUE(result);
    }

    /**
    */
    TEST(TestClassInfo, FieldInheritance)
    {
        TypeCompoundFileds instance;
        auto fields = meta::getClassAllFields<decltype(instance)>();

        const auto result = checkFields(fields,
                                        meta::FieldInfo{&TypeWithFields::intField, "intField"},
                                        meta::FieldInfo{&TypeWithFields::strField, "strField"},
                                        meta::FieldInfo{&TypeWithNamedFields::m_field1, "field1"},
                                        meta::FieldInfo{&TypeWithNamedFields::m_field2, "field2"},
                                        meta::FieldInfo{&TypeCompoundFileds::m_field3, "m_field3"},
                                        meta::FieldInfo{&TypeCompoundFileds::m_field4, "m_field4"});

        ASSERT_TRUE(result);
    }

    /**
        Test:
            Checks the direct only fields access.
            Classes that not explicitly declare NUA_CLASS_FIELDS must be non visible for fields collector.
            
    */
    TEST(TestClassInfo, DirectFields)
    {
        static_assert(meta_detail::Concept_ReflectClassFields<TypeWithNamedFields>);
        static_assert(!meta_detail::Concept_ReflectClassFields<InheritFields1>);
        static_assert(meta_detail::Concept_ReflectClassFields<InheritFields2>);

        {
            decltype(auto) originalFields = meta::getClassAllFields<TypeWithNamedFields>();
            decltype(auto) inheritedFields = meta::getClassAllFields<InheritFields1>();

            static_assert(std::tuple_size_v<decltype(originalFields)> == std::tuple_size_v<decltype(inheritedFields)>);
        }

        auto fields1 = meta::getClassDirectFields<InheritFields1>();
        auto fields2 = meta::getClassDirectFields<InheritFields2>();

        static_assert(std::tuple_size_v<decltype(fields1)> == 0);
        static_assert(std::tuple_size_v<decltype(fields2)> == 1);
    }

    /**
    */
    TEST(TestClassInfo, CheckHasMethods)
    {
        static_assert(meta::ClassHasMethods<TypeWithMethods>);
        static_assert(!meta::ClassHasMethods<TypeWithNoInfo>);
    }

    TEST(TestClassInfo, GetAllMethods)
    {
        auto methods = meta::getClassAllMethods<TypeWithMethods>();

        const auto result = checkMethods(methods,
                                         meta_detail::MethodInfoFactory<&TypeWithMethods::method1>("method1")(),
                                         meta_detail::MethodInfoFactory<&TypeWithMethods::method2>("method2")());

        ASSERT_TRUE(result);
    }

    TEST(TestClassInfo, NamedMethods)
    {
        auto methods = meta::getClassAllMethods<TypeWithNamedMethods>();

        const auto result = checkMethods(methods,
                                         meta_detail::MethodInfoFactory<&TypeWithNamedMethods::method3>("methodThird")(),
                                         meta_detail::MethodInfoFactory<&TypeWithNamedMethods::method4>("methodFourth")());

        ASSERT_TRUE(result);
    }

    TEST(TestClassInfo, MethodInheritance)
    {
        auto methods = meta::getClassAllMethods<TypeCompoundMethods>();

        const auto result = checkMethods(methods,
                                         meta_detail::MethodInfoFactory<&TypeWithMethods::method1>("method1")(),
                                         meta_detail::MethodInfoFactory<&TypeWithMethods::method2>("method2")(),
                                         meta_detail::MethodInfoFactory<&TypeWithNamedMethods::method3>("methodThird")(),
                                         meta_detail::MethodInfoFactory<&TypeWithNamedMethods::method4>("methodFourth")(),
                                         meta_detail::MethodInfoFactory<&TypeCompoundMethods::method5>("method5")(),
                                         meta_detail::MethodInfoFactory<&TypeCompoundMethods::method6>("method6")());

        ASSERT_TRUE(result);
    }

    TEST(TestClassInfo, GetDirectMethods)
    {
        auto methods = meta::getClassDirectMethods<TypeCompoundMethods>();

        const auto result = checkMethods(methods,
                                         meta_detail::MethodInfoFactory<&TypeCompoundMethods::method5>("method5")(),
                                         meta_detail::MethodInfoFactory<&TypeCompoundMethods::method6>("method6")());

        ASSERT_TRUE(result);
    }

}  // namespace nau::test
