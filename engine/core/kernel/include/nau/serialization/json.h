// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <optional>
#include <string>
#include <string_view>

#include "nau/io/stream.h"
#include "nau/kernel/kernel_config.h"
#include "nau/memory/mem_allocator.h"
#include "nau/serialization/runtime_value.h"
#include "nau/serialization/serialization.h"
#include "nau/utils/functor.h"
#include "nau/utils/result.h"

#if __has_include(<json/json.h>)
    #define HAS_JSONCPP
    #include <json/json.h>
#endif

namespace nau::serialization
{
    /**
     */
    struct JsonSettings
    {
        bool pretty = false;
        bool writeNulls = false;
    };

    /**
     */
    NAU_KERNEL_EXPORT
    Result<> jsonWrite(io::IStreamWriter&, const RuntimeValue::Ptr&, JsonSettings = {});

    /**
     */
    NAU_KERNEL_EXPORT
    Result<RuntimeValue::Ptr> jsonParse(io::IStreamReader&, IMemAllocator::Ptr = nullptr);

    /**
     */
    NAU_KERNEL_EXPORT
    Result<RuntimeValue::Ptr> jsonParseString(std::string_view, IMemAllocator::Ptr = nullptr);

    /**
     */
    inline Result<RuntimeValue::Ptr> jsonParseString(std::u8string_view str, IMemAllocator::Ptr allocator = nullptr)
    {
        return jsonParseString(std::string_view{reinterpret_cast<const char*>(str.data()), str.size()}, std::move(allocator));
    }

#ifdef HAS_JSONCPP

    /**
     */
    struct NAU_ABSTRACT_TYPE JsonValueHolder
    {
        using GetStringCallback = Functor<std::optional<std::string>(std::string_view)>;

        NAU_TYPEID(nau::serialization::JsonValueHolder)

        virtual Json::Value& getRootJsonValue() = 0;
        virtual const Json::Value& getRootJsonValue() const = 0;
        virtual Json::Value& getThisJsonValue() = 0;
        virtual const Json::Value& getThisJsonValue() const = 0;
        virtual void setGetStringCallback(GetStringCallback callback) = 0;
    };

    /**
     */
    NAU_KERNEL_EXPORT
    Json::CharReader& jsonGetParser();

    NAU_KERNEL_EXPORT
    Result<Json::Value> jsonParseToValue(std::string_view jsonString);

    inline Result<Json::Value> jsonParseToValue(std::u8string_view jsonString)
    {
        return jsonParseToValue(std::string_view{reinterpret_cast<const char*>(jsonString.data()), jsonString.size()});
    }

    /**
     */
    NAU_KERNEL_EXPORT
    RuntimeValue::Ptr jsonToRuntimeValue(Json::Value&& root, IMemAllocator::Ptr = nullptr);

    /**
     */
    inline RuntimeDictionary::Ptr jsonCreateDictionary()
    {
        return jsonToRuntimeValue(Json::Value{Json::ValueType::objectValue});
    }

    /**
     */
    inline RuntimeCollection::Ptr jsonCreateCollection()
    {
        return jsonToRuntimeValue(Json::Value{Json::ValueType::arrayValue});
    }

    /**
     */
    NAU_KERNEL_EXPORT
    RuntimeValue::Ptr jsonAsRuntimeValue(const Json::Value& root, IMemAllocator::Ptr = nullptr);

    NAU_KERNEL_EXPORT
    RuntimeValue::Ptr jsonAsRuntimeValue(Json::Value& root, IMemAllocator::Ptr = nullptr);

    NAU_KERNEL_EXPORT
    Result<> runtimeApplyToJsonValue(Json::Value& jsonValue, const RuntimeValue::Ptr&, JsonSettings = {});

    NAU_KERNEL_EXPORT
    Json::Value runtimeToJsonValue(const RuntimeValue::Ptr&, JsonSettings = {});

    NAU_KERNEL_EXPORT
    Result<> jsonWrite(io::IStreamWriter&, const Json::Value&, JsonSettings = {});

#endif

}  // namespace nau::serialization

#ifdef HAS_JSONCPP
    #undef HAS_JSONCPP
#endif