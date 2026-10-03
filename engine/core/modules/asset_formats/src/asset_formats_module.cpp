// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>

#include "animation/nanim_asset_container.h"
#include "gltf/gltf_asset_container.h"
#include "material/material_asset_container.h"
#include "nau/module/module.h"
#include "nau/service/service_provider.h"
#include "scene/scene_asset_container.h"
#include "scene/scene_container_builder.h"
#include "shader/shader_asset_container.h"
#include "texture/dds_asset_container.h"
#include "texture/texture_asset_container.h"
#include "texture/texture_asset_container_builder.h"
#include "ui/ui_asset_container.h"

namespace nau
{
    /**
     */
    class AssetFormatsModule final : public nau::DefaultModuleImpl
    {
        nau::string getModuleName() override
        {
            return u8"AssetFormatsModule";
        }

        void initialize() override
        {
            getServiceProvider().addServiceLazy([]
            {
                return std::make_unique<GltfAssetContainerLoader>();
            });

            NAU_MODULE_EXPORT_SERVICE(TextureAssetContainerLoader);
            NAU_MODULE_EXPORT_SERVICE(DDSAssetContainerLoader);
            NAU_MODULE_EXPORT_SERVICE(TextureAssetContainerBuilder);
            NAU_MODULE_EXPORT_SERVICE(ShaderAssetContainerLoader);
            NAU_MODULE_EXPORT_SERVICE(MaterialAssetContainerLoader);
            NAU_MODULE_EXPORT_SERVICE(UiAssetContainerLoader);
            NAU_MODULE_EXPORT_SERVICE(SceneContainerBuilder);
            NAU_MODULE_EXPORT_SERVICE(SceneAssetLoader);
            NAU_MODULE_EXPORT_SERVICE(NanimAssetContainerLoader);
        }
    };

}  // namespace nau

IMPLEMENT_MODULE(nau::AssetFormatsModule);
