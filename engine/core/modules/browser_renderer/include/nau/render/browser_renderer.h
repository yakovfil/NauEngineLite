// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once

#include <memory>
#include <string_view>

#include "nau/assets/cpu_mesh.h"
#include "nau/scene/components/camera_component.h"

namespace nau
{
    // All methods and destruction belong to the application worker. The DOM
    // canvas belongs to the page and must already be transferred at startup.
    class BrowserRenderer
    {
    public:
        BrowserRenderer();
        ~BrowserRenderer();
        BrowserRenderer(const BrowserRenderer&) = delete;
        BrowserRenderer& operator=(const BrowserRenderer&) = delete;
        Result<> initialize(const CpuMeshView&, std::string_view vertexShader, std::string_view fragmentShader);
        Result<> poll();
        Result<> draw(const scene::SceneObject& mesh, const scene::SceneObject& camera);
        Result<> stop();

    private:
        struct State;
        std::unique_ptr<State> m_state;
    };
}  // namespace nau
