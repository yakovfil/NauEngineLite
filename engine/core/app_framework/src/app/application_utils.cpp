// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>
#include <string_view>

#include "./logging_service.h"
#include "nau/app/global_properties.h"
#include "nau/assets/asset_db.h"
#include "nau/assets/asset_ref.h"
#include "nau/assets/scene_asset.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/scene_manager.h"

namespace nau
{
    namespace
    {
        /**
         */
        struct SceneConfig
        {
            std::string startupScene;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(startupScene))
        };
    }  // namespace

    namespace app_utils
    {
        async::Task<> loadStartupScene()
        {
            auto& serviceProvider = getServiceProvider();
            auto& globalProperties = serviceProvider.get<GlobalProperties>();
            auto sceneProperies = globalProperties.getValue<SceneConfig>("/scene");

            if (!sceneProperies)
            {
                NAU_LOG_WARNING("No default scene defined!");
                co_return;
            }

            auto startupScene = sceneProperies->startupScene;

            NAU_LOG("Loading sturtup scene: {}", startupScene);

            nau::scene::IScene::Ptr scene = co_await nau::scene::openScene(startupScene);

            if (!scene)
            {
                NAU_LOG_ERROR("Failed to load startup scene: {}", startupScene);
                co_return;
            }

            scene->setName("Startup Scene");

            co_await serviceProvider.get<scene::ISceneManager>().activateScene(std::move(scene));
        }

        nau::Result<> parseAppConfigs(const std::string& dir)
        {
            auto projectRootDir = std::filesystem::path{dir};

            if (projectRootDir.empty())
            {
                return NauMakeError("Fail to locate project root dir.");
            }

            auto wcsProjectDir = projectRootDir.wstring();
            auto utf8ProjectDir = strings::wstringToUtf8(std::wstring_view{wcsProjectDir.data(), wcsProjectDir.size()});

            GlobalProperties& globalProperties = getServiceProvider().get<GlobalProperties>();

            globalProperties.setValue("projectDir", std::move(utf8ProjectDir)).ignore();

            for (auto entry : std::filesystem::directory_iterator{projectRootDir / "config"})
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

            return ResultSuccess;
        }
    }  // namespace app_utils
}  // namespace nau
