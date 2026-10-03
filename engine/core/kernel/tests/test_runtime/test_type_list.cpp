// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// test_typelist.cpp


#include <optional>

#include "nau/rtti/type_info.h"
#include "nau/utils/type_list.h"
#include "nau/utils/type_list/contains.h"

namespace nau::test
{

    template <typename T>
    struct MakeOptional
    {
        using type = std::optional<T>;
    };

    //
    //
    // template<typename T>
    // concept IsTypeList = true;
    //
    //
    // template<template <typename> class F>
    // concept MetaFunc = requires
    //{
    //	typename F<int>::type;
    //};
    //
    //
    //
    // template<template <typename > class F, typename T>
    // struct SampleMap
    //{
    //	using type = F<T>;
    //};
    //
    //
    // template<template <typename > class F, typename T>
    // requires MetaFunc<F>
    // struct SampleMap<F, T>
    //{
    //	using type = typename F<T>::type;
    //};
    //

    TEST(TestTypeList, Transform)
    {
        using InitialTypeList = TypeList<int, unsigned, short>;
        using OptionalsTypeList = TypeList<std::optional<int>, std::optional<unsigned>, std::optional<short>>;
        using TypeList2 = type_list::TransformT<InitialTypeList, MakeOptional>;
        using TypeList3 = type_list::Transform<InitialTypeList, std::optional>;

        static_assert(std::is_same_v<TypeList2, OptionalsTypeList>);
        static_assert(std::is_same_v<TypeList3, OptionalsTypeList>);
    }

    TEST(TestTypeList, Concat)
    {
        static_assert(std::is_same_v<type_list::Concat<TypeList<>>, TypeList<>>);
        static_assert(std::is_same_v<type_list::Concat<TypeList<>, TypeList<>>, TypeList<>>);
        static_assert(std::is_same_v<type_list::Concat<TypeList<int>, TypeList<float>>, TypeList<int, float>>);
        static_assert(std::is_same_v<type_list::Concat<TypeList<unsigned, short>, TypeList<float, double>, TypeList<long>>, TypeList<unsigned, short, float, double, long>>);
    }

    TEST(TestTypeList, Distinct)
    {
        static_assert(std::is_same_v<type_list::Distinct<TypeList<>>, TypeList<>>);
        static_assert(std::is_same_v<type_list::Distinct<TypeList<int>>, TypeList<int>>);
        static_assert(std::is_same_v<type_list::Distinct<TypeList<int, int, int, int>>, TypeList<int>>);
        static_assert(std::is_same_v<type_list::Distinct<TypeList<int, float, int, float>>, TypeList<int, float>>);
    }

    TEST(TestTypeList, Contains)
    {
        using IntsList = TypeList<int, unsigned, short>;

        static_assert(type_list::Contains<IntsList, unsigned>);
        static_assert(type_list::Contains<IntsList, int>);
        static_assert(type_list::Contains<IntsList, short>);
        static_assert(!type_list::Contains<IntsList, float>);
        static_assert(!type_list::Contains<TypeList<>, float>);
    }

    // TEST(TestTypeList, ContainsAll) {
    //
    //	using IntsList = TypeList<short, int, long, unsigned short, unsigned, unsigned long>;
    //
    //	static_assert(type_list::ContainsAll<IntsList, TypeList<int, unsigned>>);
    //	static_assert(type_list::ContainsAll<IntsList, TypeList<>>);
    //	static_assert(!type_list::ContainsAll<IntsList, TypeList<unsigned, double>>);
    //	static_assert(!type_list::ContainsAll<IntsList, TypeList<float>>);
    //
    //	static_assert(!type_list::ContainsAll<TypeList<unsigned>, TypeList<unsigned, float>>);
    //	static_assert(type_list::ContainsAll<TypeList<unsigned, float>, TypeList<unsigned>>);
    //	static_assert(type_list::ContainsAll<TypeList<unsigned>, TypeList<unsigned, unsigned>>);
    // }
    //

}  // namespace nau::test
