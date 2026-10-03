// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <nau/assets/asset_db.h>

#include <memory>
#include <optional>
#include <string>

#include "nau/app/application_delegate.h"
#include "nau/app/global_properties.h"
#include "nau/app/platform_window.h"
#include "nau/app/window_manager.h"
#include "nau/assets/asset_manager.h"
#include "nau/assets/asset_ref.h"
#include "nau/assets/scene_asset.h"
#include "nau/input.h"
#include "nau/io/asset_pack_file_system.h"
#include "nau/io/virtual_file_system.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/scene_manager.h"
#include "nau/string/string_conv.h"

namespace nau
{
    class DefaultAppDelegate final : public ApplicationDelegate
    {
    public:
        DefaultAppDelegate(std::string&& modulesList) :
            m_modulesList(std::move(modulesList))
        {
        }

    private:
        std::string getModulesListString() const override
        {
#if !defined(NAU_STATIC_RUNTIME)
            return m_modulesList;
#else
            return {};
#endif
        }

        Result<> configureApplication() override
        {
            return ResultSuccess;
        }

        Result<> initializeServices() override
        {
            configureVirtualFileSystem();

            return ResultSuccess;
        }

        void onApplicationInitialized() override
        {
            auto& windowService = getServiceProvider().get<IWindowManager>();
            auto& window = windowService.getActiveWindow();
            window.setVisible(true);
            const auto [width, height] = window.getClientSize();
            input::setScreenResolution(width, height);
        }

        async::Task<> startupApplication() override
        {
            using namespace nau::scene;

            auto& props = getServiceProvider().get<GlobalProperties>();
            const std::optional<std::string> startupSceneAssetPath = props.getValue<std::string>("mainScene");
            if (!startupSceneAssetPath)
            {
                NAU_LOG_WARNING("mainScene property is not defined: do not known how start the application");
                co_return;
            }

            AssetRef<> sceneAssetRef{AssetPath{*startupSceneAssetPath}};
            SceneAsset::Ptr sceneAsset = co_await sceneAssetRef.getAssetViewTyped<SceneAsset>();
            if (!sceneAsset)
            {
                NAU_LOG_WARNING("MainScene asset:({}) is not loaded", *startupSceneAssetPath);
                co_return;
            }

            IScene::Ptr startupScene = getServiceProvider().get<ISceneFactory>().createSceneFromAsset(*sceneAsset);
            NAU_FATAL(startupScene);

            if (startupScene->getName().empty())
            {
                startupScene->setName("Startup Scene");
            }

            auto& sceneManager = getServiceProvider().get<scene::ISceneManager>();

            co_await sceneManager.activateScene(std::move(startupScene));
        }

        void configureVirtualFileSystem()
        {
            namespace fs = std::filesystem;
            auto& vfs = getServiceProvider().get<io::IVirtualFileSystem>();
            auto& assetDb = getServiceProvider().get<nau::IAssetDB>();

            fs::path currentPath = fs::current_path();

            auto& props = getServiceProvider().get<GlobalProperties>();

            if (auto contentPath = props.getValue<std::string>("contentPath"); contentPath)
            {
                const std::filesystem::path path = contentPath->c_str();
#ifdef NAU_PACKAGE_BUILD
                for (const auto& entry : fs::directory_iterator(path))
                {
                    if (entry.is_regular_file() && entry.path().extension() == ".assets")
                    {
                        const auto& filePath = entry.path();
                        auto assetPackFS = io::createAssetPackFileSystem(strings::toU8StringView(filePath.string()));
                        vfs.mount("/packs", assetPackFS).ignore();
                        assetDb.addAssetDB("packs/assets_database/database.db");
                    }
                }
#else
                auto contentFs = io::createNativeFileSystem(path.string());
                vfs.mount("/content", std::move(contentFs)).ignore();

                auto assetDbPath = path.parent_path() / "assets_database";
                vfs.mount("/assets_db", std::move(contentFs)).ignore();

                assetDb.addAssetDB("assets_db/database.db");
#endif
            }
        }

        [[maybe_unused]] std::string m_modulesList;
    };

    ApplicationDelegate::Ptr createDefaultApplicationDelegate(std::string dynModulesList)
    {
        return std::make_unique<DefaultAppDelegate>(std::move(dynModulesList));
    }
}  // namespace nau