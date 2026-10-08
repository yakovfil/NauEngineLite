// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/kernel/kernel_config.h"
#include "nau/rtti/type_info.h"
#include "nau/string/format.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"
#include "nau/utils/dag_hash.h"
#include "nau/utils/for_each_with_arg.h"
#include "nau/utils/result.h"
#include "nau/utils/scope_guard.h"
#include "nau/utils/span.h"

#define __ENUM_TO_STR(Enum, x) \
    case Enum::x:              \
        return #x;             \
        break;
#define __STR_TO_ENUM(Enum, x) \
    case #x##_h:               \
        retval = Enum::x;      \
        return true;           \
        break;  //_h will use mem_hash_fnv1

#define __ENUM_TO_STR_FUNC(Enum, ...)                               \
    inline constexpr const char* enum_to_str(Enum eval)             \
    {                                                               \
        switch (eval)                                               \
        {                                                           \
            NAU_FOR_EACH_WITH_ARG(__ENUM_TO_STR, Enum, __VA_ARGS__) \
            default:                                                \
                return "Unknown value for Enum: " #Enum;            \
        }                                                           \
    }

#define __STR_TO_ENUM_FUNC(Enum, ...)                                    \
    inline bool str_to_enum(const std::string_view& str, Enum& retval)   \
    {                                                                    \
        switch (mem_hash_fnv1(str.data(), str.size()))                   \
        {                                                                \
            NAU_FOR_EACH_WITH_ARG(__STR_TO_ENUM, Enum, __VA_ARGS__)      \
            default:                                                     \
                return false;                                            \
        }                                                                \
    }                                                                    \
    inline bool str_to_enum(const std::u8string_view& str, Enum& retval) \
    {                                                                    \
        switch (mem_hash_fnv1(str.data(), str.size()))                   \
        {                                                                \
            NAU_FOR_EACH_WITH_ARG(__STR_TO_ENUM, Enum, __VA_ARGS__)      \
            default:                                                     \
                return false;                                            \
        }                                                                \
    }

// names of registered enum values
template <typename T>
inline nau::ConstSpan<const char*> get_enum_names();

#define __ENUM_VALUE(Enum, x) Enum::x,
#define __ENUM_NAME(Enum, x) #x,

#define __CHANGE_ENUM_FUNC(Enum, ...)                                                    \
    inline void change_enum_values(Enum& retval, int enum_idx)                           \
    {                                                                                    \
        static Enum values[] = {NAU_FOR_EACH_WITH_ARG(__ENUM_VALUE, Enum, __VA_ARGS__)}; \
        retval = values[enum_idx];                                                       \
    }

#define __GET_ENUM_NAMES_FUNC(Enum, ...)                                                       \
    template <>                                                                                \
    inline nau::ConstSpan<const char*> get_enum_names<Enum>()                                  \
    {                                                                                          \
        static const char* values[] = {NAU_FOR_EACH_WITH_ARG(__ENUM_NAME, Enum, __VA_ARGS__)}; \
        return nau::ConstSpan<const char*>(values, sizeof(values) / sizeof(values[0]));        \
    }

#define __FIND_ENUM_INDEX_FUNC(Enum, ...)                                                \
    inline int find_enum_index(Enum value)                                               \
    {                                                                                    \
        static Enum values[] = {NAU_FOR_EACH_WITH_ARG(__ENUM_VALUE, Enum, __VA_ARGS__)}; \
        for (int i = 0, n = sizeof(values) / sizeof(values[0]); i < n; i++)              \
            if (values[i] == value)                                                      \
                return i;                                                                \
        return -1;                                                                       \
    }

#define __ENUM_TO_STR_FORMATTER(Enum, ...)                                      \
    template <>                                                                 \
    struct fmt::formatter<Enum> : fmt::formatter<const char*>                   \
    {                                                                           \
        auto format(Enum type, fmt::format_context& ctx) const                  \
        {                                                                       \
            return fmt::formatter<const char*>::format(enum_to_str(type), ctx); \
        }                                                                       \
    };

#define NAU_DECLARE_ENUM(etype, ...)            \
    __ENUM_TO_STR_FUNC(etype, __VA_ARGS__)      \
    __STR_TO_ENUM_FUNC(etype, __VA_ARGS__)      \
    __CHANGE_ENUM_FUNC(etype, __VA_ARGS__)      \
    __GET_ENUM_NAMES_FUNC(etype, __VA_ARGS__)   \
    __FIND_ENUM_INDEX_FUNC(etype, __VA_ARGS__)  \
    __ENUM_TO_STR_FORMATTER(etype, __VA_ARGS__) \
    __ENUM_TO_STR_FORMATTER(const etype, __VA_ARGS__)

namespace nau
{
    template <typename T>
    concept EnumValueType = std::is_enum_v<T>;

    /**
        Runtime enum information (without knowing actual C++ enum type).
     */
    struct NAU_ABSTRACT_TYPE IEnumRuntimeInfo
    {
        virtual ~IEnumRuntimeInfo() = default;

        virtual std::string_view getName() const = 0;

        virtual size_t getCount() const = 0;

        virtual std::span<const int> getIntValues() const = 0;

        virtual std::span<const std::string_view> getStringValues() const = 0;
    };

}  // namespace nau

namespace nau::nau_detail
{
    struct EnumRuntimeInfoImpl : public IEnumRuntimeInfo
    {
        const char* typeName;
        const size_t itemCount;
        std::string_view const* const strValues = nullptr;
        int const* const intValues;

        EnumRuntimeInfoImpl(const char* inTypeName, size_t inItemCount, std::string_view* inStrValues, int* inIntValues) :
            typeName(inTypeName),
            itemCount(inItemCount),
            strValues(inStrValues),
            intValues(inIntValues)
        {
        }

        std::string_view getName() const override
        {
            return typeName;
        }

        size_t getCount() const override
        {
            return itemCount;
        }

        std::span<const int> getIntValues() const override
        {
            return {intValues, itemCount};
        }

        std::span<const std::string_view> getStringValues() const override
        {
            return {strValues, itemCount};
        }
    };

    /**
        Used to "simulate" enum items values assignment.
        The problem, that NAU_DEFINE_ENUM accepts enum value items, but to refer to actual values code must
        specify enum's type names also.
        I.e
        NAU_DEFINE_ENUM_(MyEnum, Value0, Value1)
        __VA_ARGS__ will have Value0, Value1, so we need to map they to MyEnum::Value0, MyEnum::Value1, that actually near impossible (for generic cases)

        So the 'trick' used inside NAU_DEFINE_ENUM:
        - first, enum values passed as NAU_DEFINE_ENUM macro parameters treat as variables EnumValueCounter<> (i.e. EnumValueCounter<MyEnum> Value0, Value1)
        - during variables declarations EnumValueCounter will assign values same way as C++ do (Value0.value = 0, Value1.value = 1)
        - next variables maps to array of actual enum values (by EnumTraitsHelper::makeEnumValues(Value0, Value1)

        EnumValueCounter<EnumType> __VA_ARGS__;
        return EnumTraitsHelper::makeEnumValues<EnumType>(__VA_ARGS__);
     */
    template <EnumValueType EnumType, typename IntType = typename std::underlying_type_t<EnumType>>
    struct EnumValueCounter
    {
        IntType value;

        EnumValueCounter(IntType v) noexcept :
            value(v)
        {
            s_enumValueCounter = value + 1;
        }

        EnumValueCounter() noexcept :
            EnumValueCounter(s_enumValueCounter)
        {
        }

        operator IntType() const noexcept
        {
            return value;
        }

    private:
        inline static IntType s_enumValueCounter = 0;
    };

    /**
     */
    struct NAU_KERNEL_EXPORT EnumTraitsHelper
    {
        /**
            NAU_DEFINE_ENUM_(MyEnum, Value0, Value1 = 10, Value3)
            #__VA_ARGS__ will maps to string "Value0, Value1 = 10, Value3".
            parseEnumDefinition - will split that definition to array of string_view = ["Value0", "Value1", "Value3"]
        */
        static void parseEnumDefinition(std::string_view enumDefinitionString, size_t itemCount, std::string_view* result);

        /**
         */
        static std::string_view toString(IEnumRuntimeInfo& enumInfo, int value);

        static Result<int> parse(IEnumRuntimeInfo& enumInfo, std::string_view str);

        template <EnumValueType T, size_t N>
        static auto makeIntValues(const std::array<T, N>& values)
        {
            std::array<int, N> intValues;
            for (size_t i = 0; i < N; ++i)
            {
                intValues[i] = static_cast<int>(values[i]);
            }

            return intValues;
        }

        template <EnumValueType EnumType, typename... Counters>
        static auto makeEnumValues(Counters... counter)
        {
            std::array<EnumType, sizeof...(Counters)> values{static_cast<EnumType>(counter.value)...};
            return values;
        }

        template <size_t N>
        static auto makeStrValues(std::string_view (&valueDefString)[N])
        {
            std::array<std::string_view, N> result;
            for (size_t i = 0; i < N; ++i)
            {
                result[i] = parseSingleEnumEntry(valueDefString[i]);
            }

            return result;
        }
    };

}  // namespace nau::nau_detail

namespace nau
{
    template <EnumValueType EnumType>
    struct EnumTraits
    {
        static const IEnumRuntimeInfo& getRuntimeInfo()
        {
            // using ADL to find corresponding getEnumInternalInfo
            return getEnumInternalInfo(EnumType{});
        }

        static std::span<const EnumType> getValues()
        {
            decltype(auto) intern = getEnumInternalInfo(EnumType{});
            return {intern.enumValues, intern.itemCount};
        }

        static std::span<const std::string_view> getStrValues()
        {
            decltype(auto) intern = getEnumInternalInfo(EnumType{});
            return {intern.strValues, intern.itemCount};
        }

        static std::string_view toString(EnumType value)
        {
            return nau_detail::EnumTraitsHelper::toString(getEnumInternalInfo(EnumType{}), static_cast<int>(value));
        }

        static nau::Result<EnumType> parse(std::string_view str)
        {
            decltype(auto) intern = getEnumInternalInfo(EnumType{});
            nau::Result<int> parseResult = nau_detail::EnumTraitsHelper::parse(intern, str);
            NauCheckResult(parseResult);

            return static_cast<EnumType>(*parseResult);
        }
    };
}  // namespace nau

#define NAU_DEFINE_ENUM(EnumType, EnumIntType, EnumName, ...)                                              \
    enum class EnumType : EnumIntType                                                                      \
    {                                                                                                      \
        __VA_ARGS__                                                                                        \
    };                                                                                                     \
                                                                                                           \
    [[maybe_unused]] NAU_NOINLINE inline decltype(auto) getEnumInternalInfo(EnumType)                      \
    {                                                                                                      \
        using namespace nau;                                                                               \
        using namespace nau::nau_detail;                                                                   \
        using namespace std::literals;                                                                     \
                                                                                                           \
        static auto s_enumValues = EXPR_Block                                                              \
        {                                                                                                  \
            EnumValueCounter<EnumType> __VA_ARGS__;                                                        \
            return EnumTraitsHelper::makeEnumValues<EnumType>(__VA_ARGS__);                                \
        };                                                                                                 \
                                                                                                           \
        static constexpr size_t ItemCount = s_enumValues.size();                                           \
        static const std::string_view s_enumDefinitionString = #__VA_ARGS__##sv;                           \
                                                                                                           \
        static auto s_enumStrValues = EXPR_Block                                                           \
        {                                                                                                  \
            std::array<std::string_view, ItemCount> parsedValues;                                          \
            EnumTraitsHelper::parseEnumDefinition(s_enumDefinitionString, ItemCount, parsedValues.data()); \
            return parsedValues;                                                                           \
        };                                                                                                 \
                                                                                                           \
        static auto s_enumIntValues = EnumTraitsHelper::makeIntValues(s_enumValues);                       \
                                                                                                           \
        struct EnumRuntimeInfo : EnumRuntimeInfoImpl                                                       \
        {                                                                                                  \
            const EnumType* const enumValues = s_enumValues.data();                                        \
                                                                                                           \
            EnumRuntimeInfo() :                                                                            \
                EnumRuntimeInfoImpl(EnumName, ItemCount, s_enumStrValues.data(), s_enumIntValues.data())   \
            {                                                                                              \
            }                                                                                              \
        };                                                                                                 \
                                                                                                           \
        static EnumRuntimeInfo s_enumData;                                                                 \
                                                                                                           \
        return (s_enumData);                                                                               \
    }                                                                                                      \
                                                                                                           \
    [[maybe_unused]] inline NAU_NOINLINE std::string toString(EnumType value)                              \
    {                                                                                                      \
        const auto strValue = ::nau::EnumTraits<EnumType>::toString(value);                                \
        return {strValue.data(), strValue.size()};                                                         \
    }                                                                                                      \
                                                                                                           \
    [[maybe_unused]] inline NAU_NOINLINE ::nau::Result<> parse(std::string_view str, EnumType& value)      \
    {                                                                                                      \
        auto parseRes = ::nau::EnumTraits<EnumType>::parse({str.data(), str.size()});                      \
        NauCheckResult(parseRes)                                                                           \
        value = *parseRes;                                                                                 \
                                                                                                           \
        return nau::ResultSuccess;                                                                         \
    }

#define NAU_DEFINE_ENUM_(EnumType, ...) NAU_DEFINE_ENUM(EnumType, int, #EnumType, __VA_ARGS__)
