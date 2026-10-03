// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/test/helpers/app_guard.h"

#include <string>
#include <string_view>

#include "nau/module/module_manager.h"
#include "nau/service/service_provider.h"

namespace nau::test
{
    AppGuard::AppGuard(std::string modulesList) :
        m_modulesList(std::move(modulesList))
    {
    }

    AppGuard::~AppGuard()
    {
        stop();
    }

    void AppGuard::start()
    {
        m_app = createApplication(*this);
        m_app->startupOnCurrentThread();
    }

    void AppGuard::stop()
    {
        if (!m_app)
        {
            return;
        }

        m_app->stop();
        while (m_app->step())
        {
            std::this_thread::yield();
        }

        m_app.reset();
    }

    Result<> AppGuard::configureApplication()
    {
        namespace fs = std::filesystem;

        const auto projectRootDir = EXPR_Block->fs::path
        {
            // lookup project's root directory: require presence all specific directories
            // Need to distinguish the project root directory from the cmake build directory (where some directories with the same name are also present)
            const fs::path projectRelativeDir{NAU_TEST_PROJECT_SOURCE_DIR};
            const std::array requiredSubPaths = {
                fs::path{"CMakeLists.txt"},
                fs::path{"testing_content"}};

            fs::path currentPath = fs::current_path();

            do
            {
                fs::path targetPath = currentPath / projectRelativeDir;
                if (fs::exists(targetPath))
                {
                    const bool isTargetPathOk = std::all_of(requiredSubPaths.begin(), requiredSubPaths.end(), [&targetPath](const fs::path& subPath)
                    {
                        const fs::path lookupPath = targetPath / subPath;
                        return fs::exists(lookupPath);
                    });

                    if (isTargetPathOk)
                    {
                        return fs::canonical(targetPath);
                    }
                }

                currentPath = currentPath.parent_path();

            } while (currentPath.has_relative_path());

            return {};
        };

        if (projectRootDir.empty())
        {
            return NauMakeError("Fail to locate project root dir.");
        }

        auto wcsProjectDir = projectRootDir.wstring();
        auto utf8ProjectDir = strings::wstringToUtf8(std::wstring_view{wcsProjectDir.data(), wcsProjectDir.size()});

        GlobalProperties& globalProperties = getServiceProvider().get<GlobalProperties>();

        globalProperties.setValue("testProjectDir", std::move(utf8ProjectDir)).ignore();

        for (auto entry : fs::directory_iterator{projectRootDir / "config"})
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            if (strings::icaseEqual(entry.path().extension().wstring(), L".json"))
            {
                NauCheckResult(mergePropertiesFromFile(globalProperties, entry.path()));
            }
        }

        return nau::applyDefaultAppConfiguration();
    }

    Result<> AppGuard::initializeApplication()
    {
        NauCheckResult(loadModulesList(m_modulesList));

        setupTestServices();

        return ResultSuccess;
    }

}  // namespace nau::test
