// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/app/global_properties.h"
#include "nau/core_defines.h"
#include "nau/diag/logging.h"
#include "nau/io/special_paths.h"
#include "nau/module/module_manager.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_conv.h"
#include "nau/string/string_utils.h"

#if !defined(NAU_PLATFORM_WIN32) && !defined(NAU_STATIC_RUNTIME)
    #error Compilation unit is intended only for windows/ms-family platforms.
#endif

namespace nau
{
    namespace
    {
        struct EngineModulesConfig
        {
            eastl::vector<eastl::string> searchPaths;
            eastl::vector<eastl::string> optionalModules;
            bool searchEnvPath = true;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(searchPaths),
                CLASS_FIELD(optionalModules),
                CLASS_FIELD(searchEnvPath))
        };
    }  // namespace

    Result<> loadModulesList([[maybe_unused]] eastl::string_view modulesList)
    {
        namespace fs = std::filesystem;
        using namespace nau::strings;

#if !defined(NAU_STATIC_RUNTIME)
        // Get all module entries

        NAU_ASSERT(!modulesList.empty());
        if (modulesList.empty())
        {
            return NauMakeError("No modules specified, list is empty");
        }

        const EngineModulesConfig modulesConfig = EXPR_Block->EngineModulesConfig
        {
            GlobalProperties* const props = getServiceProvider().find<GlobalProperties>();
            if (!props)
            {
                return {};
            }

            return props->getValue<EngineModulesConfig>("engine/modules").value_or(EngineModulesConfig{});
        };

        const fs::path binPath = io::getKnownFolderPath(io::KnownFolder::ExecutableLocation);

        auto getAdditionalSearchPaths = [&modulesConfig, searchPaths = eastl::vector<fs::path>{}]() mutable -> const eastl::vector<fs::path>&
        {
            if (!searchPaths.empty())
            {
                return (searchPaths);
            }

            if (modulesConfig.searchEnvPath)
            {
                const DWORD len = ::GetEnvironmentVariableW(L"PATH", nullptr, 0);
                eastl::vector<wchar_t> buffer(static_cast<size_t>(len));
                ::GetEnvironmentVariableW(L"PATH", buffer.data(), len);

                for (std::wstring_view path : strings::split(std::wstring_view{buffer.data(), buffer.size()}, std::wstring_view{L";"}))
                {
                    fs::path pathDirectory{strings::trim(path)};
                    if (pathDirectory.empty())
                    {
                        continue;
                    }

                    [[maybe_unused]] std::error_code ec;
                    if (fs::exists(pathDirectory) && fs::is_directory(pathDirectory, ec))
                    {
                        searchPaths.emplace_back(std::move(pathDirectory));
                    }
                }
            }

            for (const auto& searchPath : modulesConfig.searchPaths)
            {
                const eastl::wstring wcsPath = utf8ToWString(searchPath);

                [[maybe_unused]] std::error_code ec;
                fs::path dirPath{std::wstring_view{wcsPath.c_str()}};
                if (dirPath.empty() || !fs::exists(dirPath) || !fs::is_directory(dirPath, ec))
                {
                    continue;
                }

                searchPaths.emplace_back(std::move(dirPath));
            }

            return (searchPaths);
        };

        const auto isOptionalModule = [&modulesConfig](const eastl::string_view moduleName)
        {
            return eastl::any_of(modulesConfig.optionalModules.begin(), modulesConfig.optionalModules.end(), [&moduleName](const eastl::string_view name)
            {
                return icaseEqual(moduleName, name);
            });
        };

        const auto checkFileExists = [](const fs::path& modulePath)
        {
            [[maybe_unused]] std::error_code ec;
            return fs::exists(modulePath) && fs::is_regular_file(modulePath, ec);
        };

        for (const eastl::string_view moduleName : strings::split(modulesList, eastl::string_view{","}))
        {
            eastl::wstring moduleWName = utf8ToWString(toU8StringView(strings::trim(moduleName)));
            if (moduleWName.empty())
            {
                continue;
            }

            const fs::path moduleDllName = fs::path(std::wstring_view{moduleWName.data(), moduleWName.size()}).replace_extension(L".dll");

            fs::path moduleFullPath = binPath / moduleDllName;

            bool modulePathExists = checkFileExists(moduleFullPath);
            if (!modulePathExists)
            {
                NAU_LOG_WARNING(u8"Module ({}) not found at ({}), attempt to search at alternative locations", eastl::string{moduleName}, wstringToUtf8(toStringView(binPath.wstring())));
                for (const fs::path& additionalPath : getAdditionalSearchPaths())
                {
                    moduleFullPath = additionalPath / moduleDllName;
                    if (modulePathExists = checkFileExists(moduleFullPath); modulePathExists)
                    {
                        break;
                    }
                }
            }

            if (!modulePathExists)
            {
                if (!isOptionalModule(moduleName))
                {
                    NAU_FAILURE_ALWAYS("Module's ({}) location not found", eastl::string{moduleName});
                    return NauMakeError("Module's ({}) location found", eastl::string{moduleName});
                }
                else
                {
                    NAU_LOG_INFO(u8"Optional Module ({}) not found", eastl::string{moduleName});
                    continue;
                }
            }

            const std::wstring wcsFullPath = moduleFullPath.wstring();

            if (auto loadResult = getModuleManager().loadModule(moduleName, wcsFullPath); !loadResult)
            {
                if (!isOptionalModule(moduleName))
                {
                    NAU_FAILURE_ALWAYS("Required module  ({}) is not loaded:({})", eastl::string{moduleName}, loadResult.getError()->getMessage());
                    return loadResult.getError();
                }
                else
                {
                    NAU_LOG_ERROR("Optional module ({}) is not loaded:({})", eastl::string{moduleName}, loadResult.getError()->getMessage());
                    continue;
                }
            }

            NAU_LOG(u8"Loading module ({}) at ({})", eastl::string{moduleName}, wstringToUtf8(toStringView(wcsFullPath)));
        }
#endif

        return ResultSuccess;
    }
}  // namespace nau
