// nau/module/internal/module_entry.h
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//
#pragma once

#include <memory>
#include <string>

#include "nau/io/file_system.h"
#include "nau/kernel/kernel_config.h"

struct IModule;

namespace nau
{
    struct ModuleEntry
    {
        std::string name;
        std::shared_ptr<::nau::IModule> iModule = nullptr;
        bool isModuleInitialized = false;

#if !NAU_STATIC_RUNTIME
        std::wstring dllPath;
        HMODULE dllHandle = 0;
#endif
    };

}  // namespace nau
