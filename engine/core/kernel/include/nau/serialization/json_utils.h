// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/serialization/json_utils.h

#pragma once
#if !__has_include(<json/json.h>)
    #error json cpp required
#endif

#include <string>
#include <string_view>
#include <type_traits>

#include "nau/io/stream_utils.h"
#include "nau/memory/mem_allocator.h"
#include "nau/serialization/json.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/string/string_conv.h"

namespace nau::serialization
{

    /*
     */
    struct JsonUtils
    {
        /**

        */
        template <typename T>
        static inline Result<T> parse(std::reference_wrapper<const Json::Value> jValue)
        {
            // rtstack();
            return runtimeValueCast<T>(jsonAsRuntimeValue(jValue, getDefaultAllocator()));
        }

        /**

        */
        template <typename T>
        static inline Result<> parse(T& value, std::reference_wrapper<const Json::Value> jValue)
        {
            // rtstack();
            return runtimeValueApply(value, jsonAsRuntimeValue(jValue, getDefaultAllocator()));
        }

        /**

        */
        template <typename T>
        static inline Result<> parse(T& value, std::u8string_view jsonString)
        {
            // rtstack();

            Result<RuntimeValue::Ptr> parseResult = jsonParseString(jsonString, getDefaultAllocator());
            if (!parseResult)
            {
                return parseResult.getError();
            }

            return runtimeValueApply(value, *parseResult);
        }

        /**
         */
        template <typename T>
        static inline Result<T> parse(std::u8string_view jsonString)
        {
            // rtstack();

            Result<RuntimeValue::Ptr> parseResult = jsonParseString(jsonString, getDefaultAllocator());
            if (!parseResult)
            {
                return parseResult.getError();
            }

            return runtimeValueCast<T>(*parseResult);
        }

        template <typename T>
        static inline Result<> parse(T& value, std::string_view jsonString)
        {
            if (jsonString.empty())
            {
                return NauMakeError("Empty string");
            }

            const char8_t* const utfPtr = reinterpret_cast<const char8_t*>(jsonString.data());
            return parse(value, std::u8string_view{utfPtr, jsonString.size()});
        }

        template <typename T>
        static inline Result<T> parse(std::string_view jsonString)
        {
            if (jsonString.empty())
            {
                return NauMakeError("Empty string");
            }

            const char8_t* const utfPtr = reinterpret_cast<const char8_t*>(jsonString.data());
            return parse<T>(std::u8string_view{utfPtr, jsonString.size()});
        }

        /**
         */
        template <typename Char = char8_t>
        static inline std::basic_string<Char> stringify(const auto& value, JsonSettings settings = {})
        {
            // rtstack();
            std::basic_string<Char> buffer;
            io::InplaceStringWriter<Char> writer{buffer};
            serialization::jsonWrite(writer, makeValueRef(value, getDefaultAllocator()), settings).ignore();

            return buffer;
        }

        /**
         */
        template <typename T>
        static inline Json::Value toJsonValue(const T& value)
        {
            // rtstack();
            return runtimeToJsonValue(makeValueRef(value, getDefaultAllocator()));
        }
    };

}  // namespace nau::serialization
