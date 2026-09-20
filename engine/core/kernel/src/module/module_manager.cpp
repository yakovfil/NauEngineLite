// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/module/module_manager.h"

#include <EASTL/map.h>

#include <filesystem>

#include "nau/io/file_system.h"
#include "nau/memory/singleton_memop.h"
#include "nau/module/internal/module_entry.h"
#include "nau/module/module.h"
#if !defined(NAU_STATIC_RUNTIME)
#include "nau/platform/windows/diag/win_error.h"
#endif
#include "nau/string/hash_string.h"
#include "nau/string/string_conv.h"

#if defined(NAU_STATIC_RUNTIME)
namespace nau::module_detail
{
    extern void initializeAllStaticModules(nau::IModuleManager* manager);

}  // namespace nau::module_detail
#endif

using createModuleFunctionPtr = nau::IModule* (*)(void);

namespace nau
{
    class ModuleManagerImpl final : public IModuleManager
    {
        NAU_DECLARE_SINGLETON_MEMOP(ModuleManagerImpl)
    public:
        ModuleManagerImpl()
        {
            NAU_ASSERT(s_instance == nullptr);
            s_instance = this;
        }

        ~ModuleManagerImpl()
        {
            doCleanup();

            NAU_ASSERT(s_instance == this);
            s_instance = nullptr;
        }

        void doModulesPhase(ModulesPhase phase) override
        {
            if (phase == ModulesPhase::Init)
            {
                doInit();
            }
            else if (phase == ModulesPhase::PostInit)
            {
                doPostInit();
            }
            else if (phase == ModulesPhase::Cleanup)
            {
                doCleanup();
            }
        }

        void registerModule(const char* moduleName, eastl::shared_ptr<::nau::IModule> inModule) override
        {
            lock_(m_mutex);

            nau::string moduleNameString = moduleName;  // TODO: too many memory allocation
            nau::hash_string hName = moduleNameString;

            NAU_ASSERT(!moduleRegistry.count(hName), u8"Module already registered");

            moduleRegistry[hName] = nau::ModuleEntry{
                .name = nau::string(moduleName),
                .iModule = inModule,
                .isModuleInitialized = false};
        }

        bool isModuleLoaded(const char* moduleName) override
        {
            nau::string moduleNameString = moduleName;  // TODO: too many memory allocation
            nau::hash_string hName = moduleNameString;

            return isModuleLoaded(hName);
        }

        bool isModuleLoaded(const nau::hash_string& moduleName) override
        {
            lock_(m_mutex);

            return moduleRegistry.count(moduleName);
        }

        eastl::shared_ptr<IModule> getModule(nau::hash_string moduleName) override
        {
            lock_(m_mutex);

            NAU_ASSERT(moduleRegistry.count(moduleName), u8"This module is not registered");
            const ModuleEntry& entry = moduleRegistry[moduleName];

            NAU_ASSERT(entry.isModuleInitialized, u8"Module is not initialized");
            return entry.iModule;
        }

        eastl::shared_ptr<IModule> getModuleInitialized(nau::hash_string moduleName) override
        {
            lock_(m_mutex);

            NAU_ASSERT(moduleRegistry.count(moduleName), u8"This module is not registered");
            ModuleEntry& entry = moduleRegistry[moduleName];

            if (!entry.isModuleInitialized)
            {
                entry.iModule->initialize();
                entry.isModuleInitialized = true;
            }

            return entry.iModule;
        }

#if !defined(NAU_STATIC_RUNTIME)
        Result<> loadModule(const nau::string& name, const nau::string& dllPath) override
        {
            namespace fs = std::filesystem;

            lock_(m_mutex);

            nau::hash_string hName = name;

            if (moduleRegistry.count(hName))
            {
                // Module already loaded
                return ResultSuccess;
            }

            eastl::wstring dllWStringPath = strings::utf8ToWString(dllPath);

            HMODULE hmodule = GetModuleHandleW(dllWStringPath.c_str());
            if (!hmodule)
            {
                hmodule = LoadLibraryW(dllWStringPath.c_str());
                if (!hmodule)
                {
                    const auto errCode = diag::getAndResetLastErrorCode();
                    NAU_LOG_ERROR("Fail to load library ({}) with error:()", dllPath, diag::getWinErrorMessageA(errCode));
                    return NauMakeError("Fail to load library ({})({}):({})", dllPath.tostring(), errCode, diag::getWinErrorMessageA(errCode));
                    
                }
            }

            ModuleEntry entry{};
            entry.name = name;
            entry.dllPath = dllWStringPath;
            entry.dllHandle = hmodule;

            createModuleFunctionPtr createModuleFuncPtr = (createModuleFunctionPtr)GetProcAddress(entry.dllHandle, "createModule");
            if (!createModuleFuncPtr)
            {
                NAU_LOG_ERROR("Can't get proc address, module:({})", dllPath);
                return NauMakeError("Module does not export 'createModule' function");
            }

            eastl::shared_ptr<IModule> iModule = eastl::shared_ptr<IModule>(createModuleFuncPtr());
            if (!iModule)
            {
                NAU_LOG_ERROR("Invalid module object, module:({})", dllPath);
                return NauMakeError("Invalid module object");
            }

            entry.iModule = iModule;
            moduleRegistry[hName] = entry;

            if (m_needInitializeNewModules)
            {
                moduleRegistry[hName].isModuleInitialized = true;
                moduleRegistry[hName].iModule->initialize();
            }

            return ResultSuccess;
        }
#endif

    private:
        inline static ModuleManagerImpl* s_instance = nullptr;

        void doInit()
        {
#if defined(NAU_STATIC_RUNTIME)  // TODO: maybe move this two blocks to other function
            module_detail::initializeAllStaticModules(this);
#endif

            lock_(m_mutex);
            for (auto& [hash, moduleEntry] : moduleRegistry)
            {
                if (!moduleEntry.isModuleInitialized)
                {
                    NAU_ASSERT(moduleEntry.iModule, u8"Module wasn't created");
                    moduleEntry.iModule->initialize();
                    moduleEntry.isModuleInitialized = true;
                }
            }

            m_needInitializeNewModules = true;
        }

        void doPostInit()
        {
            lock_(m_mutex);

            for (auto const& [hash, moduleEntry] : moduleRegistry)
            {
                moduleEntry.iModule->postInit();
            }
        }

        void doCleanup()
        {
            lock_(m_mutex);

            for (auto const& [hash, moduleEntry] : moduleRegistry)
            {
                moduleEntry.iModule->deinitialize();
            }
        }

        eastl::map<nau::hash_string, ModuleEntry> moduleRegistry;
        std::mutex m_mutex;
        bool m_needInitializeNewModules = false;

        friend IModuleManager& ::nau::getModuleManager();
        friend bool ::nau::hasModuleManager();
    };

    IModuleManager::Ptr createModuleManager()
    {
        return eastl::make_unique<ModuleManagerImpl>();
    }

    IModuleManager& getModuleManager()
    {
        NAU_FATAL(ModuleManagerImpl::s_instance);
        return *ModuleManagerImpl::s_instance;
    }

    bool hasModuleManager()
    {
        return ModuleManagerImpl::s_instance != nullptr;
    }

}  // namespace nau
