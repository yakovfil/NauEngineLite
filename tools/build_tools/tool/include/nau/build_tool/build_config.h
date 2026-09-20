// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <string>

#ifndef NAU_STATIC_RUNTIME

    #ifdef _MSC_VER
        #ifdef BUILD_TOOL_EXPORT
            #define BUILD_TOOL_API __declspec(dllexport)
        #else
            #define BUILD_TOOL_API __declspec(dllimport)
        #endif

    #else
        #error Unknown Compiler/OS
    #endif

#else
    #define BUILD_TOOL_API
#endif

namespace nau
{
    struct BUILD_TOOL_API BuildConfig
    {
        std::string projectPath;
        std::string targetDestination;
        std::string buildConfiguration;
        bool openAfterBuild;
        bool compileSources;
        bool compileAssets;
        bool forceCopyBinaries;
        std::string preset = "win_vs2026_x64_dll";
    };

    enum class BUILD_TOOL_API BuildResult
    {
        Success,
        Failed,
        Invalid
    };
};  // namespace nau