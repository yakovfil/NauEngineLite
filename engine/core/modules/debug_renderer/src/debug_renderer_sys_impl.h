// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string_view>
#include <vector>

#include "graphics_assets/material_asset.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/assets/asset_manager.h"
#include "nau/debugRenderer/debug_render_system.h"

namespace nau
{
    class DebugRenderSysImpl : public DebugRenderSystem,
                               public IServiceInitialization,
                               public IServiceShutdown
    {
        NAU_RTTI_CLASS(nau::DebugRenderSysImpl, DebugRenderSystem, IServiceInitialization, IServiceShutdown)

        DebugRenderSysImpl() = default;

    private:
        static constexpr std::string_view PrimitivePipelineName = "line";
        static constexpr std::string_view MeshPipelineName = "mesh";

        MaterialAssetView::Ptr m_debugMaterial;

#pragma region Primitives

        struct LinesDescription
        {
            struct LineDescription
            {
                math::Point3 pointA;
                math::Point3 pointB;
                float timer;
                math::E3DCOLOR color;
            };

            std::vector<math::Point3> pointA;
            std::vector<math::Point3> pointB;
            std::vector<float> timer;
            std::vector<math::E3DCOLOR> color;
            size_t size()
            {
                return pointA.size();
            }

            void resize(size_t n)
            {
                pointA.resize(n);
                pointB.resize(n);
                timer.resize(n, -1);  // negative time not drawing
                color.resize(n);
            }

            void clear()
            {
                pointA.clear();
                pointB.clear();
                timer.clear();
                color.clear();
            }
            void set(size_t n, const LineDescription& line)
            {
                pointA[n] = line.pointA;
                pointB[n] = line.pointB;
                timer[n] = line.timer;
                color[n] = line.color;
            }

            void set(size_t i, size_t j)
            {
                pointA[i] = pointA[j];
                pointB[i] = pointB[j];
                timer[i] = timer[j];
                color[i] = color[j];
            }
        };

        nau::threading::SpinLock m_linesRWMutex;
        LinesDescription m_lines;
        std::vector<uint32_t> m_freeLineIndexes;

        Sbuffer* m_verticesPrimPositionBuffer;
        Sbuffer* m_verticesPrimColorBuffer;
        int m_maxPointsCount = 1024 * 1024 * 8;
        int m_currentPointsCount = 0;

        bool m_primDirtyFlag = false;

#pragma endregion Primitives

#pragma region Meshes

        struct MeshConstData
        {
            math::Matrix4 Transform;
            math::Color4 color;
        };

        struct MeshInfo
        {
            Sbuffer* verticesBuffer;
            Sbuffer* indeciesBuffer;
            MeshConstData transformColor;
            float timer = -1;
            size_t triNum = 0;
        };

        nau::threading::SpinLock m_meshRWMutex;
        std::vector<MeshInfo> m_meshesInfo;

#pragma endregion Meshes

    protected:
        void DrawPrimitives(const math::Matrix4& cameraMatrix, float dTime);
        void DrawQuads(){};
        void DrawMeshes(const math::Matrix4& cameraMatrix, float dTime);

        void UpdateLinesBuffer(float dTime);
        void UpdateMeshesBuffers(float dTime);

    public:
        async::Task<> preInitService() override;
        async::Task<> initService() override;
        async::Task<> shutdownService() override;
        std::vector<const rtti::TypeInfo*> getServiceDependencies() const override;

        virtual ~DebugRenderSysImpl() override = default;

        virtual void draw(const math::Matrix4& cameraMatrix, float dTime) override;
        virtual void clear() override;

    public:
        virtual void drawLine(const math::Point3& pos0, const math::Point3& pos1, const math::Color4& color, float time) override;

        virtual void drawBoundingBox(const math::BBox3& box, const math::Color4& color, float time) override;
        virtual void drawBoundingBox(const math::BBox3& box, const math::Matrix4& transform, const math::Color4& color, float time) override;
        virtual void drawArrow(const math::Point3& p0, const math::Point3& p1, const math::Color4& color, const math::Vector3& n, float time) override;
        virtual void drawPoint(const math::Point3& pos, const float& size, float time) override;
        virtual void drawCircle(const double& radius, const math::Color4& color, const math::Matrix4& transform, int density, float time) override;
        virtual void drawSphere(const double& radius, const math::Color4& color, const math::Matrix4& transform, int density, float time) override;
        virtual void drawPlane(const math::Vector4& p, const math::Color4& color, float sizeWidth, float sizeNormal, bool drawCenterCross, float time) override;
        virtual void drawFrustrum(const math::Matrix4& view, const math::Matrix4& proj, float time) override;

        virtual void drawStaticMesh(const StaticMesh& mesh, const math::Matrix4& transform, const math::Color4& color, float time) override;
    };

}  // namespace nau