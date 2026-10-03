// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// lua_toolkit/lua_interop.h


#pragma once

#ifdef __clang__
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wshadow-uncaptured-local"
#endif  // __clang__

#include <memory>

#include "lua_toolkit/lua_headers.h"
#include "lua_toolkit/lua_toolkit_config.h"
#include "nau/dispatch/class_descriptor.h"
#include "nau/dispatch/dispatch.h"

namespace nau::lua
{
    /**
     */
    NAU_LUATOOLKIT_EXPORT
    nau::Ptr<> makeValueFromLuaStack(lua_State* l, int index, IMemAllocator::Ptr = nullptr);

    // NAU_LUATOOLKIT_EXPORT
    // nau::Ptr<> makeRefValueFromLuaStack(lua_

    NAU_LUATOOLKIT_EXPORT
    Result<> pushRuntimeValue(lua_State* l, const RuntimeValue::Ptr& value);

    NAU_LUATOOLKIT_EXPORT
    Result<> initializeClass(lua_State* l, IClassDescriptor::Ptr classDescriptor, bool keepMetatableOnStack);

    NAU_LUATOOLKIT_EXPORT
    Result<> pushObject(lua_State* l, nau::Ptr<> object, IClassDescriptor::Ptr classDescriptor);

    NAU_LUATOOLKIT_EXPORT
    Result<> pushObject(lua_State* l, std::unique_ptr<IRttiObject> object, IClassDescriptor::Ptr classDescriptor);

    NAU_LUATOOLKIT_EXPORT
    Result<> pushDispatch(lua_State* l, nau::Ptr<> dispatch);

    NAU_LUATOOLKIT_EXPORT
    Result<> populateTable(lua_State*, int index, const RuntimeValue::Ptr& value);

    /**
     */
    template <typename T>
    inline nau::Result<> cast(lua_State* l, int index, T& value)
    {
        using namespace nau;

        // auto& allocator = GetRtStackAllocator();
        auto& allocator = getDefaultAllocator();

        return nau::RuntimeValue::assign(
            makeValueRef(value, allocator),
            makeValueFromLuaStack(l, index, allocator));
    }

    /**
     */
    template <typename T>
    inline nau::Result<T> cast(lua_State* l, int index)
    {
        static_assert(std::is_default_constructible_v<T>, "Requires default constructor");

        T value{};
        NauCheckResult(cast(l, index, value)) return value;
    }

}  // namespace nau::lua
