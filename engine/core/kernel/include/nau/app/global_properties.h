// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <filesystem>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>

#include "nau/diag/logging.h"
#include "nau/io/stream.h"
#include "nau/memory/mem_allocator.h"
#include "nau/memory/stack_allocator.h"
#include "nau/rtti/type_info.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/utils/functor.h"

namespace nau
{
    /**
        @brief Application global properties access
     */
    struct NAU_ABSTRACT_TYPE GlobalProperties
    {
        NAU_TYPEID(nau::GlobalProperties)

        using ModificationLock = std::unique_lock<std::shared_mutex>;
        using VariableResolverCallback = Functor<std::optional<std::string>(std::string_view)>;

        virtual ~GlobalProperties() = default;

        /**
            @brief get read-only property at path as runtime value
            @param path Property path. Can be compound: with sections separated by '/': "app/section_0/prop_1"
            @return value as RuntimeValue or null if property does not exists
        */
        virtual RuntimeValue::Ptr getRead(std::string_view path, IMemAllocator::Ptr allocator = nullptr) const = 0;

        /**
            @brief checks the property at path exists within dictionary
            @param path Property path.
        */
        virtual bool contains(std::string_view path) const = 0;

        /**
            @brief setting property value at path
                Existing primitive value (numbers, strings, booleans) in path will be reset to the new one,
            but collections (arrays and objects/dictionaries) will be merged.

            @param path Property path.
        */
        virtual Result<> set(std::string_view path, RuntimeValue::Ptr value) = 0;

        /**
            @brief get property as modifiable runtime value
            @param path Property path.
            @param lock the synchronization mutex
        */
        virtual Result<RuntimeValue::Ptr> getModify(std::string_view path, ModificationLock& lock, IMemAllocator::Ptr allocator = nullptr) = 0;

        /**
            @brief reads and parses a stream, then applies all the properties it retrieves to the dictionary.
        */
        // virtual Result<> mergeFromStream(io::IStreamReader& stream, std::string_view contentType = "application/json") = 0;

        /**
            @brief applies all the properties from values
        */
        virtual Result<> mergeWithValue(const RuntimeValue& value) = 0;

        /**
         */
        virtual void addVariableResolver(std::string_view kind, VariableResolverCallback resolver) = 0;

        /**
            @brief getting typed value
         */
        template <RuntimeValueRepresentable T>
        requires(std::is_default_constructible_v<T>)
        std::optional<T> getValue(std::string_view path);

        /**
            @brief setting typed value
         */
        template <RuntimeValueRepresentable T>
        Result<> setValue(std::string_view path, const T& value);
    };

    template <RuntimeValueRepresentable T>
    requires(std::is_default_constructible_v<T>)
    std::optional<T> GlobalProperties::getValue(std::string_view path)
    {
        StackAllocatorUnnamed;

        auto value = getRead(path, nullptr);
        if (!value)
        {
            return std::nullopt;
        }

        T resultValue;
        if (const Result<> applyResult = runtimeValueApply(resultValue, value); !applyResult)
        {
            NAU_LOG_WARNING("Fail to apply property value at path({}):{}", path, applyResult.getError()->getMessage());
            return std::nullopt;
        }

        return resultValue;
    }

    template <RuntimeValueRepresentable T>
    Result<> GlobalProperties::setValue(std::string_view path, const T& value)
    {
        StackAllocatorUnnamed;

        return set(path, makeValueRef(value));
    }

    /**
        @brief reads and parses a stream, then applies all the properties it retrieves to the properties dictionary.
    */
    Result<> mergePropertiesFromStream(GlobalProperties& properties, io::IStreamReader& stream, std::string_view contentType = "application/json");

    /**
        @brief reads and parses a file, then applies all the properties it retrieves to the properties dictionary.
    */
    Result<> mergePropertiesFromFile(GlobalProperties& properties, const std::filesystem::path& filePath, std::string_view contentType = {});

    /**
        @brief Serialize properties content into the specified stream.
     */
    void dumpPropertiesToStream(GlobalProperties& properties, io::IStreamWriter& stream, std::string_view contentType = "application/json");

    /**
        @brief Serialize properties content into the string.
     */
    std::string dumpPropertiesToString(GlobalProperties& properties, std::string_view contentType = "application/json");

}  // namespace nau
