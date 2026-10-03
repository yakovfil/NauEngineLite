// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <string_view>

namespace nau::shader_globals
{
    NAU_RENDER_EXPORT bool containsName(std::string_view name);

    NAU_RENDER_EXPORT void addVariable(std::string_view name, size_t size, const void* defaultValue = nullptr);

    NAU_RENDER_EXPORT void setVariable(std::string_view name, const void* value);
    NAU_RENDER_EXPORT void getVariable(std::string_view name, size_t* size, void** value);
} // namespace nau::shaderGlobals
