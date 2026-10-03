// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/framework/framework_config.h


#if !defined(NAU_STATIC_RUNTIME)

    #ifdef _MSC_VER
        #ifdef NAU_FRAMEWORK_BUILD
            #define NAU_FRAMEWORK_EXPORT __declspec(dllexport)
        #else
            #define NAU_FRAMEWORK_EXPORT __declspec(dllimport)
        #endif

    #else
        #error Unknown Compiler/OS
    #endif

#else
    #define NAU_FRAMEWORK_EXPORT
#endif
