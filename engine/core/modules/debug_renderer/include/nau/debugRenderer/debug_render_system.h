// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <memory>
#include <vector>

#include "nau/math/dag_bounds3.h"
#include "nau/math/dag_color.h"
#include "nau/math/math.h"
#include "nau/service/service.h"

namespace nau
{
    class NAU_ABSTRACT_TYPE DebugRenderSystem : public IRttiObject
    {
        NAU_INTERFACE(nau::DebugRenderSystem, IRttiObject)

        using Ptr = std::unique_ptr<DebugRenderSystem>;

    public:
        virtual ~DebugRenderSystem() = default;

        virtual void draw(const math::Matrix4& cameraMatrix, float dTime) = 0;
        virtual void clear() = 0;

        virtual void drawLine(const math::Point3& pos0, const math::Point3& pos1, const math::Color4& color, float time) = 0;

        virtual void drawBoundingBox(const math::BBox3& box, const math::Color4& color, float time) = 0;
        virtual void drawBoundingBox(const math::BBox3& box, const math::Matrix4& transform, const math::Color4& color, float time) = 0;
        virtual void drawArrow(const math::Point3& p0, const math::Point3& p1, const math::Color4& color, const math::Vector3& n, float time) = 0;
        virtual void drawPoint(const math::Point3& pos, const float& size, float time) = 0;
        virtual void drawCircle(const double& radius, const math::Color4& color, const math::Matrix4& transform, int density, float time) = 0;
        virtual void drawSphere(const double& radius, const math::Color4& color, const math::Matrix4& transform, int density, float time) = 0;
        virtual void drawPlane(const math::Vector4& p, const math::Color4& color, float sizeWidth, float sizeNormal, bool drawCenterCross, float time) = 0;
        virtual void drawFrustrum(const math::Matrix4& view, const math::Matrix4& proj, float time) = 0;

        // TODO Add drawing quad on screen support.
        //  Currently working with texture-assets is tedious for DebugRender purposes.
        // virtual void DrawTextureOnScreen(Texture* tex, int x, int y, int width, int height, int zOrder) = 0;

        struct StaticMesh
        {
            const std::vector<math::Point3>& vertices;
            const std::vector<uint32_t>& indices;
        };

        virtual void drawStaticMesh(const StaticMesh& mesh, const math::Matrix4& transform, const math::Color4& color, float time) = 0;
    };

    NAU_DEBUGRENDERER_EXPORT DebugRenderSystem& getDebugRenderer();
    NAU_DEBUGRENDERER_EXPORT void setDebugRenderer(DebugRenderSystem::Ptr);
    NAU_DEBUGRENDERER_EXPORT DebugRenderSystem::Ptr createDebugRenderer();

}  // namespace nau