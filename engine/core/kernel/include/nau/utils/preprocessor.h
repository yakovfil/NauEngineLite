// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#define CONCAT_IMPL__(s1, s2) s1##s2
#define PP_CONCATENATE(s1, s2) CONCAT_IMPL__(s1, s2)

#define PP_STRINGIZE(M) #M
#define PP_STRINGIZE_VALUE(M) PP_STRINGIZE(M)

#ifdef __COUNTER__
    #define ANONYMOUS_VAR(Prefix) PP_CONCATENATE(Prefix, __COUNTER__)
#else
    #define ANONYMOUS_VAR(Prefix) PP_CONCATENATE(Prefix, _LINE__)
#endif

// #define WFILE CONCATENATE(L, __FILE__)
// #define WFUNCTION CONCATENATE(L, __FUNCTION__)

//

#if defined(_MSC_VER) && !defined(__clang__)
    #if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
        #define PP_VA_OPT_AVAILABLE 0
    #else
        #define PP_VA_OPT_AVAILABLE 1
    #endif

#elif defined(__clang__)

    #define PP_VA_OPT_AVAILABLE 1

#endif
/*
#if !defined(PP_VA_OPT_AVAILABLE) || !PP_VA_OPT_AVAILABLE
#error setup for other compilers
#endif
*/


#if defined(__EMSCRIPTEN__)
#define NAU_PLATFORM_HEADER_DIR_NAME emscripten
#else
#define NAU_PLATFORM_HEADER_DIR_NAME windows
#endif
#define NAU_PLATFORM_PATH_IMPL(Platform, FileName) nau/platform/Platform/FileName
#define NAU_PLATFORM_PATH(FileName) NAU_PLATFORM_PATH_IMPL(NAU_PLATFORM_HEADER_DIR_NAME, FileName)

#define NAU_PLATFORM_HEADER(FileName) PP_STRINGIZE_VALUE(NAU_PLATFORM_PATH(FileName))
