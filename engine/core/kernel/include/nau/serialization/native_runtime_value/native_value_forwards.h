// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <concepts>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>

#include "nau/memory/mem_allocator.h"
#include "nau/meta/class_info.h"
#include "nau/serialization/native_runtime_value/type_info_value.h"
#include "nau/serialization/runtime_value.h"
#include "nau/utils/tuple_utility.h"
#include "nau/utils/type_list/fill.h"
#include "nau/utils/type_utility.h"

namespace nau::ser_detail
{
    template <typename T>
    struct KnownSetCollection : std::false_type
    {
    };

    template <typename T, typename... Traits>
    struct KnownSetCollection<std::set<T, Traits...>> : std::true_type
    {
    };

    template <typename T, typename... Traits>
    struct KnownSetCollection<std::unordered_set<T, Traits...>> : std::true_type
    {
    };

}  // namespace nau::ser_detail

/**

*/

namespace nau
{
    /**
     */
    template <typename T>
    concept LikeStdCollection = requires(const T& collection) {
        typename T::value_type;
        typename T::size_type;
        { collection.size() } -> std::same_as<typename T::size_type>;
        { collection.begin() } -> std::template same_as<typename T::const_iterator>;
        { collection.front() } -> std::template same_as<typename T::const_reference>;
        { collection.back() } -> std::template same_as<typename T::const_reference>;
    } && requires(T& collection) {
        collection.clear();
        { collection.begin() } -> std::template same_as<typename T::iterator>;
        { collection.front() } -> std::template same_as<typename T::reference>;
        { collection.back() } -> std::template same_as<typename T::reference>;
        collection.emplace_back();  // } -> std::template same_as<typename T::reference>;
    };

    /**
     */
    template <typename T>
    concept LikeStdVector = requires(const T& collection) {
        typename T::value_type;
        typename T::size_type;
        { collection.operator[](0) } -> std::template same_as<typename T::const_reference>;
    } && requires(T& collection) {
        { collection.operator[](0) } -> std::template same_as<typename T::reference>;
    } && LikeStdCollection<T>;

    /**
     */
    template <typename T>
    concept LikeStdList = requires(T& collection) {
        collection.emplace_front();
    } && LikeStdCollection<T>;

    /**
     */
    template <typename T>
    concept LikeStdMap = requires(const T& dict) {
        typename T::key_type;
        typename T::value_type;

        { dict.size() } -> std::same_as<typename T::size_type>;
        { dict.begin() } -> std::same_as<typename T::const_iterator>;
        { dict.end() } -> std::same_as<typename T::const_iterator>;
        { dict.find(constLValueRef<typename T::key_type>()) } -> std::same_as<typename T::const_iterator>;
    } && requires(T& dict) {
        dict.clear();
        { dict.begin() } -> std::same_as<typename T::iterator>;
        { dict.end() } -> std::same_as<typename T::iterator>;
        { dict.find(constLValueRef<typename T::key_type>()) } -> std::same_as<typename T::iterator>;
        dict.try_emplace(constLValueRef<typename T::key_type>());
    };

    /**
        Using restricted set types to minimize collisions with other declarations.
    */
    template <typename Collection>
    concept LikeSet = ser_detail::KnownSetCollection<Collection>::value;

    /**
     */
    template <typename T>
    concept LikeStdOptional = requires(const T& opt) {
        { opt.has_value() } -> std::template same_as<bool>;
    } && requires(T& opt) {
        opt.reset();
        opt.emplace();
        { opt.value() } -> std::template same_as<typename T::value_type&>;
    };

    /**
     */
    template <typename T>
    concept StringParsable = requires(const T& value) {
        {
            toString(value)
        } -> std::same_as<std::string>;
    } && requires(T& value) {
        {
            parse(std::string_view{}, value)
        } -> std::same_as<Result<>>;
    };

    template <typename T>
    concept WithOwnRuntimeValue = requires {
        T::HasOwnRuntimeValue;
    } && T::HasOwnRuntimeValue;

    template <typename T>
    concept AutoStringRepresentable = StringParsable<T> && !WithOwnRuntimeValue<T>;

    /**
     */
    template <typename>
    struct TupleValueOperations1 : std::false_type
    {
    };

    template <typename>
    struct UniformTupleValueOperations : std::false_type
    {
    };

    template <typename... T>
    struct TupleValueOperations1<std::tuple<T...>> : std::true_type
    {
        static inline constexpr size_t TupleSize = sizeof...(T);

        using Type = std::tuple<T...>;
        using Elements = TypeList<T...>;

        template <size_t Index>
        static decltype(auto) element(Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid tuple element index");
            return std::get<Index>(tup);
        }

        template <size_t Index>
        static decltype(auto) element(const Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid tuple element index");
            return std::get<Index>(tup);
        }
    };

    template <>
    struct TupleValueOperations1<std::tuple<>> : std::true_type
    {
        static inline constexpr size_t TupleSize = 0;

        using Type = std::tuple<>;
        using Elements = TypeList<>;

        template <size_t Index>
        static std::nullopt_t element(const Type&)
        {
            static_assert(Index == 0, "Invalid tuple element index");
            return std::nullopt;
        }
    };

    template <typename First, typename Second>
    struct TupleValueOperations1<std::pair<First, Second>>
    {
        static constexpr size_t TupleSize = 2;

        using Type = std::pair<First, Second>;
        using Elements = TypeList<First, Second>;

        template <size_t Index>
        static decltype(auto) element(Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid pair index");
            return std::get<Index>(tup);
        }

        template <size_t Index>
        static decltype(auto) element(const Type& tup)
        {
            static_assert(Index < TupleSize, "Invalid pair index");
            return std::get<Index>(tup);
        }
    };

    template <typename T, size_t Size>
    struct UniformTupleValueOperations<std::array<T, Size>> : std::true_type
    {
        static constexpr size_t TupleSize = Size;
        using Type = std::array<T, Size>;
        using Element = T;

        static_assert(Size > 0, "std::array<T,0> does not supported");

        static decltype(auto) element(Type& tup, size_t index)
        {
            NAU_ASSERT(index < Size, "[{}], size():{}", index, TupleSize);
            return tup[index];
        }

        static decltype(auto) element(const Type& tup, size_t index)
        {
            NAU_ASSERT(index < Size, "[{}], size():{}", index, TupleSize);
            return tup[index];
        }
    };

    template <typename T>
    concept NauClassWithFields = meta::ClassHasFields<T>;

    template <typename T>
    concept LikeTuple = TupleValueOperations1<T>::value;

    template <typename T>
    concept LikeUniformTuple = UniformTupleValueOperations<T>::value;

    RuntimeValueRef::Ptr makeValueRef(const RuntimeValue::Ptr& value, IMemAllocator::Ptr = nullptr);

    RuntimeValueRef::Ptr makeValueRef(RuntimeValue::Ptr& value, IMemAllocator::Ptr = nullptr);

    // Integral
    template <std::integral T>
    RuntimeIntegerValue::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <std::integral T>
    RuntimeIntegerValue::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <std::integral T>
    RuntimeIntegerValue::Ptr makeValueCopy(T, IMemAllocator::Ptr = nullptr);

    // Boolean
    RuntimeBooleanValue::Ptr makeValueRef(bool&, IMemAllocator::Ptr = nullptr);

    RuntimeBooleanValue::Ptr makeValueRef(const bool&, IMemAllocator::Ptr = nullptr);

    RuntimeBooleanValue::Ptr makeValueCopy(bool, IMemAllocator::Ptr = nullptr);

    // Floating point
    template <std::floating_point T>
    RuntimeFloatValue::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <std::floating_point T>
    RuntimeFloatValue::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <std::floating_point T>
    RuntimeFloatValue::Ptr makeValueCopy(T, IMemAllocator::Ptr = nullptr);

    // String
    template <typename... Traits>
    RuntimeStringValue::Ptr makeValueRef(std::basic_string<char, Traits...>&, IMemAllocator::Ptr = nullptr);

    template <typename... Traits>
    RuntimeStringValue::Ptr makeValueRef(const std::basic_string<char, Traits...>&, IMemAllocator::Ptr = nullptr);

    RuntimeStringValue::Ptr makeValueCopy(std::string_view, IMemAllocator::Ptr = nullptr);

    template <typename C, typename... Traits>
    requires(sizeof(C) == sizeof(char))
    RuntimeStringValue::Ptr makeValueRef(std::basic_string<C, Traits...>&, IMemAllocator::Ptr = nullptr);

    template <typename C, typename... Traits>
    requires(sizeof(C) == sizeof(char))
    RuntimeStringValue::Ptr makeValueRef(const std::basic_string<C, Traits...>&, IMemAllocator::Ptr = nullptr);

    // template <typename... Traits>
    // RuntimeStringValue::Ptr makeValueRef(std::basic_string<char8_t, Traits...>&, IMemAllocator::Ptr = nullptr);

    // template <typename... Traits>
    // RuntimeStringValue::Ptr makeValueRef(const std::basic_string<char8_t, Traits...>&, IMemAllocator::Ptr = nullptr);

    RuntimeStringValue::Ptr makeValueCopy(std::u8string_view, IMemAllocator::Ptr = nullptr);

    template <AutoStringRepresentable T>
    RuntimeStringValue::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <AutoStringRepresentable T>
    RuntimeStringValue::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <AutoStringRepresentable T>
    RuntimeStringValue::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <AutoStringRepresentable T>
    RuntimeStringValue::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    // Optional
    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdOptional T>
    RuntimeOptionalValue::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    // Tuple
    template <LikeTuple T>
    RuntimeReadonlyCollection::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <LikeTuple T>
    RuntimeReadonlyCollection::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeTuple T>
    RuntimeReadonlyCollection::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeTuple T>
    RuntimeReadonlyCollection::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    template <LikeUniformTuple T>
    RuntimeReadonlyCollection::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <LikeUniformTuple T>
    RuntimeReadonlyCollection::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeUniformTuple T>
    RuntimeReadonlyCollection::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeUniformTuple T>
    RuntimeReadonlyCollection::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    // Collection
    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdVector T>
    RuntimeCollection::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeStdList T>
    RuntimeCollection::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueRef(T&, IMemAllocator::Ptr = nullptr);

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueRef(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueCopy(const T&, IMemAllocator::Ptr = nullptr);

    template <LikeSet T>
    RuntimeCollection::Ptr makeValueCopy(T&&, IMemAllocator::Ptr = nullptr);

    // Dictionary
    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueRef(T& dict, IMemAllocator::Ptr = nullptr);

    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueRef(const T& dict, IMemAllocator::Ptr = nullptr);

    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueCopy(const T& dict, IMemAllocator::Ptr = nullptr);

    template <LikeStdMap T>
    RuntimeDictionary::Ptr makeValueCopy(T&& dict, IMemAllocator::Ptr = nullptr);

    // Object
    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueRef(T& obj, IMemAllocator::Ptr = nullptr);

    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueRef(const T& obj, IMemAllocator::Ptr = nullptr);

    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueCopy(const T& obj, IMemAllocator::Ptr = nullptr);

    template <NauClassWithFields T>
    RuntimeObject::Ptr makeValueCopy(T&& obj, IMemAllocator::Ptr = nullptr);

}  // namespace nau

namespace nau::ser_detail
{
    template <typename T>
    decltype(makeValueRef(lValueRef<T>()), std::true_type{}) hasMakeValueRefChecker(int);

    template <typename>
    std::false_type hasMakeValueRefChecker(...);

    template <typename T>
    decltype(makeValueCopy(constLValueRef<T>()), std::true_type{}) hasMakeValueCopyChecker(int);

    template <typename>
    std::false_type hasMakeValueCopyChecker(...);

    template <typename T>
    constexpr bool HasRtValueRepresentationHelper = decltype(hasMakeValueRefChecker<T>(int{0}))::value;

    template <typename T>
    constexpr bool HasRtValueCopyHelper = std::is_copy_constructible_v<T> && decltype(hasMakeValueCopyChecker<T>(int{0}))::value;

    template <typename... T>
    inline consteval bool allHasRuntimeValueRepresentation(TypeList<T...>)
    {
        return (HasRtValueRepresentationHelper<T> && ...);
    }

    template <typename T>
    consteval bool hasRtValueRepresentation()
    {
        return HasRtValueRepresentationHelper<T>;
    }

    template <LikeStdOptional T>
    consteval bool hasRtValueRepresentation()
    {
        using ValueType = typename T::value_type;
        return HasRtValueRepresentationHelper<ValueType>;
    }

    template <LikeTuple T>
    consteval bool hasRtValueRepresentation()
    {
        using Elements = typename TupleValueOperations1<T>::Elements;
        return allHasRuntimeValueRepresentation(Elements{});
    }

    template <LikeUniformTuple T>
    consteval bool hasRtValueRepresentation()
    {
        using Element = typename UniformTupleValueOperations<T>::Element;
        return HasRtValueRepresentationHelper<Element>;
    }

    template <LikeStdCollection T>
    consteval bool hasRtValueRepresentation()
    {
        return HasRtValueRepresentationHelper<typename T::value_type>;
    }

    template <LikeSet T>
    consteval bool hasRtValueRepresentation()
    {
        return HasRtValueRepresentationHelper<typename T::value_type>;
    }

    template <typename T>
    consteval bool hasMakeValueCopy()
    {
        return HasRtValueCopyHelper<T>;
    }

}  // namespace nau::ser_detail

namespace nau
{

    template <typename T>
    inline constexpr bool HasRuntimeValueRepresentation = ser_detail::hasRtValueRepresentation<T>();

    template <typename T>
    inline constexpr bool HasMakeValueCopy = ser_detail::hasMakeValueCopy<T>();

    template <typename T>
    concept RuntimeValueRepresentable = ser_detail::hasRtValueRepresentation<T>();

}  // namespace nau
