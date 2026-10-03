// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <memory>
#include <string_view>

#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_object.h"
#include "nau/string/hash_string.h"
#include "nau/utils/result.h"

namespace nau
{
    struct IModule;

    /**
     */
    struct NAU_ABSTRACT_TYPE IModuleManager
    {
        enum class ModulesPhase
        {
            Init,
            PostInit,
            Cleanup
        };

        using Ptr = std::unique_ptr<IModuleManager>;

        virtual ~IModuleManager() = default;

        virtual void doModulesPhase(ModulesPhase phase) = 0;

        virtual void registerModule(const char* moduleName, std::shared_ptr<::nau::IModule> inModule) = 0;

        virtual bool isModuleLoaded(const char* moduleName) = 0;
        virtual bool isModuleLoaded(const nau::hash_string& moduleName) = 0;

        virtual std::shared_ptr<IModule> getModule(nau::hash_string moduleName) = 0;

        template <typename T>
        std::shared_ptr<T> getModule(nau::hash_string moduleName)
        {
            return getModule(moduleName);
        }

        virtual std::shared_ptr<IModule> getModuleInitialized(nau::hash_string moduleName) = 0;

        template <typename T>
        std::shared_ptr<T> getModuleInitialized(nau::hash_string moduleName)
        {
            return getModuleInitialized(moduleName);
        }

#if !NAU_STATIC_RUNTIME
        // For runtime module loading
        virtual Result<> loadModule(const nau::string& name, const nau::string& dllPath) = 0;
#endif
    };

    NAU_KERNEL_EXPORT IModuleManager::Ptr createModuleManager();

    NAU_KERNEL_EXPORT IModuleManager& getModuleManager();

    NAU_KERNEL_EXPORT bool hasModuleManager();

    NAU_KERNEL_EXPORT Result<> loadModulesList(std::string_view moduleList);
}  // namespace nau
