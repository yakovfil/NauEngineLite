// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

/*
 namespace nau
{
    namespace scene
    {
        // don't make nested namespace (nau::scene) since this file is used by shader compiler
    }
}
*/

// Common (C++ and HLSL) block.

#define CUBE_FACE_COUNT 6
#define CS_ENV_CUBEMAPS_BLOCK_SIZE 8
#define CUBEMAP_ENV_FACE_SIZE 1024
#define IRRADIANCE_MAP_FACE_SIZE 128
#define REFLECTION_MAP_FACE_SIZE 512
#define REFLECTION_MAP_MIP_COUNT 10

#define NAU_MAX_SKINNING_BONES_COUNT 128

#define SHADING_NORMAL 0
#define SHADING_EMISSIVE 3
#define MAX_EMISSION 4.0f

#define GLOBAL_BUFFER_PREFIX GB_
#define SYSTEM_BUFFER_PREFIX SB_

#ifdef __cplusplus
    #include <string_view>

// C++ only block.

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

namespace nau::shader_defines
{
    constexpr std::string_view GlobalBufferPrefix = STRINGIFY(GLOBAL_BUFFER_PREFIX);
    constexpr std::string_view SystemBufferPrefix = STRINGIFY(SYSTEM_BUFFER_PREFIX);

    inline bool isGlobalBuffer(std::string_view bufferName)
    {
        return bufferName.starts_with(GlobalBufferPrefix);
    }

    inline bool isSystemBuffer(std::string_view bufferName)
    {
        return bufferName.starts_with(SystemBufferPrefix);
    }
}

#else // __cplusplus

// HLSLonly block.

#define CONCAT_(prefix, name) prefix##name
#define CONCAT(prefix, name) CONCAT_(prefix, name)

#define GLOBAL_CBUFFER(name) cbuffer CONCAT(GLOBAL_BUFFER_PREFIX, name)
#define SYSTEM_CBUFFER(name) cbuffer CONCAT(SYSTEM_BUFFER_PREFIX, name)

#endif // __cplusplus