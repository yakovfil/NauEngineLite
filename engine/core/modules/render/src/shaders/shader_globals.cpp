// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "nau/shaders/shader_globals.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nau::shader_globals
{
    namespace detail
    {
        std::unordered_map<std::string, std::vector<std::byte>> g_shaderDataTable;

        std::shared_mutex g_mutex;
    } // namespace detail

    using namespace detail;

    bool containsName(std::string_view name)
    {
        shared_lock_(g_mutex);
        return detail::g_shaderDataTable.contains(std::string(name));
    }

    void addVariable(std::string_view name, size_t size, const void* defaultValue /* = nullptr */)
    {
        NAU_ASSERT(size);

        lock_(g_mutex);

        g_shaderDataTable[std::string(name)] = {};
        g_shaderDataTable[std::string(name)].resize(size);

        if (defaultValue != nullptr)
        {
            memcpy(g_shaderDataTable[std::string(name)].data(), defaultValue, size);
        }
    }

    void setVariable(std::string_view name, const void* value)
    {
        NAU_FATAL(g_shaderDataTable.contains(std::string(name)), "Global shader variable not found: {}", name.data());

        NAU_ASSERT(value);

        auto& buf = g_shaderDataTable[std::string(name)];
        memcpy(buf.data(), value, buf.size());
    }

    void getVariable(std::string_view name, size_t* size, void** value)
    {
        NAU_FATAL(g_shaderDataTable.contains(std::string(name)), "Global shader variable not found: {}", name.data());

        NAU_ASSERT(size);
        NAU_ASSERT(value);

        auto& buf = g_shaderDataTable[std::string(name)];
        *size = buf.size();
        *value = buf.data();
    }
} // namespace nau::shaderGlobals
