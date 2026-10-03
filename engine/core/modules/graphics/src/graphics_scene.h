// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <shared_mutex>
#include <span>
#include <unordered_map>
#include <vector>

#include "graphics_assets/material_asset.h"
#include "graphics_assets/skinned_mesh_asset.h"
#include "graphics_assets/static_mesh_asset.h"
#include "graphics_nodes.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/math/math.h"
#include "nau/scene/camera/camera_manager.h"
#include "nau/scene/components/scene_component.h"
#include "nau/scene/scene_processor.h"
#include "nau/shaders/shader_defines.h"
#include "render/lights/clusteredLights.h"
#include "render_pipeline/billboards_manager.h"
#include "render_pipeline/skinned_mesh_manager.h"
#include "render_pipeline/static_mesh_manager.h"

namespace nau::scene
{
    class CameraComponent;
}


namespace nau
{
    class GraphicsScene
    {
    public:
        GraphicsScene(const GraphicsScene&) = delete;
        GraphicsScene(Uid worldUid);

        async::Task<> initialize();

        async::Task<> activateComponents(std::span<const scene::Component*> components, async::Task<> barrier);

        async::Task<> deactivateComponents(std::span<const scene::DeactivatedComponentData> components);

        async::Task<> update();

        void renderFrame(bool withGBuffer = false);
        void renderDepth();
        void renderOutlineMask();
        void renderTranslucency();
        void renderLights();
        void renderDebugLights();
        void renderSceneDebug();
        void renderBillboards();

        void syncSceneState();

        CameraNode& getMainCamera();
        bool hasCamera();
        bool hasMainCamera() const;

        bool hasDirectionalLight();
        const std::vector<DirectionalLightNode>& getDirectionalLights() const;

        bool hasEnvironmentNode();
        EnvironmentNode& getEnvironmentNode();
        
        void setObjectHighlight(nau::Uid uid, bool flag);

        nau::RenderScene* getRenderScene();

    private:
        void syncSceneCameras();

        const Uid m_worldUid;

        std::vector<StaticMeshNode> m_staticMeshes;
        std::vector<BillboardNode> m_billboards;
        std::vector<SkinnedMeshNode> m_skinnedMeshes;
        std::vector<DirectionalLightNode> m_directionalLights;
        std::vector<EnvironmentNode> m_envNodes;
        std::vector<LightNode> m_lightNodes;
        std::unordered_map<Uid, CameraNode> m_cameras;

        render::ClusteredLights m_lights;
        
        nau::Ptr<nau::RenderScene> m_renderScene;

        shaders::RenderStateId fakeId;
        scene::ICameraManager::CamerasSnapshot m_allInGameCameras;
    };
}  // namespace nau
