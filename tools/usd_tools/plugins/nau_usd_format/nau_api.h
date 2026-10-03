// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#ifndef NAU_STATIC_RUNTIME

    #ifdef _MSC_VER
        #ifdef USD_FORMAT_EXPORT
            #define USD_FORMAT_API __declspec(dllexport)
        #else
            #define USD_FORMAT_API __declspec(dllimport)
        #endif

    #else
        #error Unknown Compiler/OS
    #endif

#else
    #define USD_FORMAT_API
#endif
