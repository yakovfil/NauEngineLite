// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "debug_renderer_sys_impl.h"

#include <memory>
#include <numbers>
#include <vector>

#include "graphics_assets/shader_asset.h"
#include "nau/assets/asset_manager.h"
#include "nau/assets/asset_ref.h"
#include "nau/graphics/core_graphics.h"
#include "nau/service/service_provider.h"

#define MULTIPLICATION_LINES_ARRAY_INCREASE  0.3
#define FIXED_LINES_ARRAY_INCREASE  100

namespace nau
{
    using namespace math;

    DebugRenderSystem& getDebugRenderer()
    {
        return getServiceProvider().get<DebugRenderSystem>();
    }

    void setDebugRenderer(DebugRenderSystem::Ptr ptr)
    {
        getServiceProvider().addService(std::move(ptr));
    };

    DebugRenderSystem::Ptr createDebugRenderer()
    {
        return std::make_unique<DebugRenderSysImpl>();
    };

    async::Task<> DebugRenderSysImpl::preInitService()
    {
        return IServiceInitialization::preInitService();
    }

    async::Task<> DebugRenderSysImpl::initService()
    {
        auto debugMatRef = MaterialAssetRef{"file:/res/materials/embedded/debug_renderer.nmat_json"};
        m_debugMaterial = co_await debugMatRef.getAssetViewTyped<MaterialAssetView>();

        m_verticesPrimPositionBuffer = d3d::create_vb(m_maxPointsCount * sizeof(Point3), SBCF_DYNAMIC, u8"DebugVertexPositionBuf");
        m_verticesPrimColorBuffer = d3d::create_vb(m_maxPointsCount * sizeof(Color4), SBCF_DYNAMIC, u8"DebugVertexColorBuf");

        co_return;
    }

    async::Task<> DebugRenderSysImpl::shutdownService()
    {
        m_verticesPrimPositionBuffer->destroy();
        m_verticesPrimColorBuffer->destroy();

        for (auto& meshInfo : m_meshesInfo)
        {
            meshInfo.verticesBuffer->destroy();
            meshInfo.indeciesBuffer->destroy();
        }
        m_meshesInfo.clear();

        m_debugMaterial.reset();

        return async::Task<>::makeResolved();
    }

    std::vector<const rtti::TypeInfo*> DebugRenderSysImpl::getServiceDependencies() const
    {
        return {&rtti::getTypeInfo<ICoreGraphics>()};
    }

    void DebugRenderSysImpl::DrawPrimitives(const math::Matrix4& cameraMatrix, float dTime)
    {
        UpdateLinesBuffer(dTime);

        if (m_currentPointsCount == 0)
        {
            return;
        }

        d3d::set_vs_const(0, reinterpret_cast<const void*>(&cameraMatrix), 4);

        m_debugMaterial->bindPipeline(PrimitivePipelineName);

        d3d::setvsrc(0, m_verticesPrimPositionBuffer, sizeof(Point3));
        d3d::setvsrc(1, m_verticesPrimColorBuffer, sizeof(Color4));

        d3d::draw(PRIM_LINELIST, 0, m_currentPointsCount / 2);
    }

    void DebugRenderSysImpl::DrawMeshes(const math::Matrix4& cameraMatrix, float dTime)
    {
        lock_(m_meshRWMutex);

        if (m_meshesInfo.empty())
        {
            return;
        }

        UpdateMeshesBuffers(dTime);
        
        m_debugMaterial->bindPipeline(MeshPipelineName);

        d3d::set_vs_const(0, reinterpret_cast<const void*>(&cameraMatrix), 4);
        for (int i = 0; i < m_meshesInfo.size(); ++i)
        {
            d3d::set_vs_const(4, reinterpret_cast<const void*>(&m_meshesInfo[i].transformColor), 5);

            d3d::setvsrc(0, m_meshesInfo[i].verticesBuffer, sizeof(Point3));
            d3d::setind(m_meshesInfo[i].indeciesBuffer);

            d3d::drawind(PRIM_TRILIST, 0, m_meshesInfo[i].triNum, 0);
        }
    }

    void DebugRenderSysImpl::UpdateMeshesBuffers(float dTime)
    {
        int freeIndex = 0;
        for (int i = 0; i < m_meshesInfo.size(); ++i)
        {
            if (m_meshesInfo[i].timer > 0)
            {
                m_meshesInfo[i].timer -= dTime;

                if (i != freeIndex)
                {
                    std::swap(m_meshesInfo[i], m_meshesInfo[freeIndex]);
                }
                freeIndex++;
            }
        }
        for (int i = freeIndex; i < m_meshesInfo.size(); ++i)
        {
            m_meshesInfo[i].indeciesBuffer->destroy();
            m_meshesInfo[i].verticesBuffer->destroy();
        }
        m_meshesInfo.resize(freeIndex);
    }

    void DebugRenderSysImpl::UpdateLinesBuffer(float dTime)
    {
        lock_(m_linesRWMutex);

        if (m_primDirtyFlag)
        {
            if (m_freeLineIndexes.size() > MULTIPLICATION_LINES_ARRAY_INCREASE * m_lines.size() + 2*FIXED_LINES_ARRAY_INCREASE)
            {
                size_t freeCell = 0;
                for (int i = 0; i < m_lines.size(); ++i)
                {
                    if (m_lines.timer[i] > 0)
                    {
                        if (i != freeCell)
                        {
                            m_lines.set(freeCell, i);
                            m_lines.timer[i] = -1;
                        }
                        freeCell++;
                    }
                }
                size_t newSize = freeCell;
                m_lines.resize(newSize);
                m_freeLineIndexes.clear();
            }

            m_primDirtyFlag = false;

            m_currentPointsCount = (m_lines.size() - m_freeLineIndexes.size()) * 2;

            if (m_currentPointsCount > m_maxPointsCount)
            {
                m_maxPointsCount = alignedSize(m_currentPointsCount + m_currentPointsCount * MULTIPLICATION_LINES_ARRAY_INCREASE, 1024);
                m_verticesPrimPositionBuffer->destroy();
                m_verticesPrimColorBuffer->destroy();
                m_verticesPrimPositionBuffer = d3d::create_vb(m_maxPointsCount * sizeof(Point3), SBCF_DYNAMIC, u8"DebugVertexPositionBuf");
                m_verticesPrimColorBuffer = d3d::create_vb(m_maxPointsCount * sizeof(Color4), SBCF_DYNAMIC, u8"DebugVertexColorBuf");
            }

            if (m_currentPointsCount > 0)
            {
                Point3* vertexPositionMem = nullptr;
                Color4* vertexColorMem = nullptr;

                m_verticesPrimPositionBuffer->lock(0, m_currentPointsCount * sizeof(Point3),
                    reinterpret_cast<void**>(&vertexPositionMem), VBLOCK_WRITEONLY);
                m_verticesPrimColorBuffer->lock(0, m_currentPointsCount * sizeof(Color4),
                    reinterpret_cast<void**>(&vertexColorMem), VBLOCK_WRITEONLY);

                size_t vertexPosition = 0;
                for (int i = 0; i < m_lines.size(); ++i)
                {
                    if (m_lines.timer[i] >= 0)
                    {
                        vertexPositionMem[i * 2] = m_lines.pointA[i];
                        vertexPositionMem[i * 2 + 1] = m_lines.pointB[i];
                        vertexColorMem[i * 2] = m_lines.color[i];
                        vertexColorMem[i * 2 + 1] = m_lines.color[i];
                        vertexPosition++;
                    }
                }

                m_verticesPrimPositionBuffer->unlock();
                m_verticesPrimColorBuffer->unlock();
            }
        }

        for (int i = 0; i < m_lines.size(); ++i)
        {
            if (m_lines.timer[i] < 0)
            {
                continue;
            }
            m_lines.timer[i] -= dTime;
            if (m_lines.timer[i] < 0)
            {
                m_primDirtyFlag = true;
                m_freeLineIndexes.push_back(i);
            }
        }
    }


    void DebugRenderSysImpl::draw(const math::Matrix4& cameraMatrix, float dTime)
    {
        d3d::setwire(true);

        DrawPrimitives(cameraMatrix, dTime);
        DrawQuads();
        DrawMeshes(cameraMatrix, dTime);

        d3d::setwire(false);
    }

    void DebugRenderSysImpl::clear()
    {
        lock_(m_meshRWMutex);
        lock_(m_linesRWMutex);

        if (m_lines.size() == 0 && m_freeLineIndexes.size() == 0)
        {
            return;
        }

        m_lines.clear();
        m_freeLineIndexes.clear();
        m_primDirtyFlag = true;
    }

    void DebugRenderSysImpl::drawStaticMesh(const StaticMesh& mesh, const math::Matrix4& transform, const math::Color4& color, float time)
    {
        if (time < 0)
        {
            return;
        }

        lock_(m_meshRWMutex);
        MeshInfo meshInfo;

        meshInfo.verticesBuffer = d3d::create_vb(mesh.vertices.size() * sizeof(Point3), SBCF_DYNAMIC);
        Point3* vertexMem = nullptr;
        meshInfo.verticesBuffer->lock(0, mesh.vertices.size() * sizeof(Point3),
                                      reinterpret_cast<void**>(&vertexMem), VBLOCK_WRITEONLY);
        memcpy_s(vertexMem, mesh.vertices.size() * sizeof(Point3), mesh.vertices.data(), mesh.vertices.size() * sizeof(Point3));
        meshInfo.verticesBuffer->unlock();

        meshInfo.indeciesBuffer = d3d::create_ib(mesh.indices.size() * sizeof(uint32_t), SBCF_DYNAMIC | SBCF_INDEX32);
        uint32_t* indexMem = nullptr;
        meshInfo.indeciesBuffer->lock32(0, mesh.indices.size() * sizeof(uint32_t),
                                        &indexMem, VBLOCK_WRITEONLY);
        memcpy_s(indexMem, mesh.indices.size() * sizeof(uint32_t), mesh.indices.data(), mesh.indices.size() * sizeof(uint32_t));
        meshInfo.indeciesBuffer->unlock();

        meshInfo.triNum = mesh.indices.size() / 3;
        meshInfo.timer = time;
        meshInfo.transformColor = {transform, color};
        m_meshesInfo.push_back(meshInfo);
    };

    void DebugRenderSysImpl::drawBoundingBox(const BBox3& box, const math::Color4& color, float time)
    {
        Point3 corners[BBox3::PointsCount];

        for (int k = 0; k < BBox3::PointsCount; ++k)
        {
            corners[k] = box.point(k);
        }

        drawLine(corners[0], corners[1], color, time);
        drawLine(corners[0], corners[2], color, time);
        drawLine(corners[2], corners[3], color, time);
        drawLine(corners[3], corners[1], color, time);

        drawLine(corners[4], corners[5], color, time);
        drawLine(corners[4], corners[6], color, time);
        drawLine(corners[6], corners[7], color, time);
        drawLine(corners[7], corners[5], color, time);

        drawLine(corners[0], corners[4], color, time);
        drawLine(corners[1], corners[5], color, time);
        drawLine(corners[2], corners[6], color, time);
        drawLine(corners[3], corners[7], color, time);
    }

    void DebugRenderSysImpl::drawBoundingBox(const BBox3& box, const Matrix4& transform, const math::Color4& color, float time)
    {
        Point3 corners[BBox3::PointsCount];

        for (int k = 0; k < BBox3::PointsCount; ++k)
        {
            corners[k] = box.point(k);
        }

        for (auto& corner : corners)
        {
            corner = transform * corner;
        }

        drawLine(corners[0], corners[1], color, time);
        drawLine(corners[1], corners[2], color, time);
        drawLine(corners[2], corners[3], color, time);
        drawLine(corners[3], corners[0], color, time);

        drawLine(corners[4], corners[5], color, time);
        drawLine(corners[5], corners[6], color, time);
        drawLine(corners[6], corners[7], color, time);
        drawLine(corners[7], corners[4], color, time);

        drawLine(corners[0], corners[4], color, time);
        drawLine(corners[1], corners[5], color, time);
        drawLine(corners[2], corners[6], color, time);
        drawLine(corners[3], corners[7], color, time);
    }

    void DebugRenderSysImpl::drawLine(const math::Point3& pos0, const math::Point3& pos1, const math::Color4& color, float time)
    {
        if (time < 0)
        {
            return;
        }
        lock_(m_linesRWMutex);
        if (!m_freeLineIndexes.size())
        {
            size_t newSize = m_lines.size() +  m_lines.size() * MULTIPLICATION_LINES_ARRAY_INCREASE + FIXED_LINES_ARRAY_INCREASE;
            for (int k = newSize; k > m_lines.size(); --k)
            {
                m_freeLineIndexes.push_back(k - 1);
            }
            m_lines.resize(newSize);
        }
        auto n = m_freeLineIndexes.back();
        m_freeLineIndexes.pop_back();

        m_lines.set(n, {Point3(pos0),
                        Point3(pos1),
                        time,
                        e3dcolor(color)});

        m_primDirtyFlag = true;
    }

    void DebugRenderSysImpl::drawArrow(const Point3& p0, const Point3& p1, const Color4& color, const Vector3& n, float time)
    {
        drawLine(p0, p1, color, time);

        auto a = lerp(p0, p1, 0.85f);

        auto diff = p1 - p0;
        auto side = cross(n, diff) * 0.05f;

        drawLine(a + side, p1, color, time);
        drawLine(a - side, p1, color, time);
    }

    void DebugRenderSysImpl::drawPoint(const Point3& pos, const float& size, float time)
    {
        drawLine(pos + Vector3(size, 0, 0), pos - Vector3(size, 0, 0), {1.0f, 0.0f, 0.0f, 1.0f}, time);
        drawLine(pos + Vector3(0, size, 0), pos - Vector3(0, size, 0), {0.0f, 1.0f, 0.0f, 1.0f}, time);
        drawLine(pos + Vector3(0, 0, size), pos - Vector3(0, 0, size), {0.0f, 0.0f, 1.0f, 1.0f}, time);
    }

    void DebugRenderSysImpl::drawCircle(const double& radius, const Color4& color, const Matrix4& transform, int density, float time)
    {
        double angleStep = std::numbers::pi_v<float> * 2 / density;

        for (int i = 0; i < density; i++)
        {
            auto point0X = radius * cos(angleStep * i);
            auto point0Y = radius * sin(angleStep * i);

            auto point1X = radius * cos(angleStep * (i + 1));
            auto point1Y = radius * sin(angleStep * (i + 1));

            auto p0 = transform * Point3(static_cast<float>(point0X), static_cast<float>(point0Y), 0);
            auto p1 = transform * Point3(static_cast<float>(point1X), static_cast<float>(point1Y), 0);

            drawLine(p0, p1, color, time);
        }
    }

    void DebugRenderSysImpl::drawSphere(const double& radius, const Color4& color, const Matrix4& transform, int density, float time)
    {
        drawCircle(radius, color, transform, density, time);
        drawCircle(radius, color, transform*Matrix4::rotationX(0.5 * std::numbers::pi_v<float>), density, time);
        drawCircle(radius, color, transform*Matrix4::rotationY(0.5 * std::numbers::pi_v<float>), density, time);
    }

    void DebugRenderSysImpl::drawPlane(const Vector4& p, const Color4& color, float sizeWidth, float sizeNormal, bool drawCenterCross, float time)
    {
        auto dir = Vector3(p.getX(), p.getY(), p.getZ());
        if (lengthSqr(dir) == 0.0f)
        {
            return;
        }

        normalize(dir);

        auto up = Vector3(0, 0, 1);
        auto right = cross(dir, up);
        if (lengthSqr(right) < MATH_SMALL_NUMBER)
        {
            up = Vector3(0, 1, 0);
            right = cross(dir, up);
        }
        normalize(right);

        up = cross(right, dir);

        Point3 pos = Point3(-dir * p.getW());

        auto leftPoint = pos - right * sizeWidth;
        auto rightPoint = pos + right * sizeWidth;
        auto downPoint = pos - up * sizeWidth;
        auto upPoint = pos + up * sizeWidth;

        drawLine(leftPoint + up * sizeWidth, rightPoint + up * sizeWidth, color, time);
        drawLine(leftPoint - up * sizeWidth, rightPoint - up * sizeWidth, color, time);
        drawLine(downPoint - right * sizeWidth, upPoint - right * sizeWidth, color, time);
        drawLine(downPoint + right * sizeWidth, upPoint + right * sizeWidth, color, time);

        if (drawCenterCross)
        {
            drawLine(leftPoint, rightPoint, color, time);
            drawLine(downPoint, upPoint, color, time);
        }

        drawPoint(pos, 0.5f, time);
        drawArrow(pos, pos + dir * sizeNormal, color, right, time);
    }

    void DebugRenderSysImpl::drawFrustrum(const Matrix4& view, const Matrix4& proj, float time)
    {
        const auto viewProj = proj * view;
        const auto inv = inverse(viewProj);

        std::vector<Point3> corners;
        for (unsigned int x = 0; x < 2; ++x)
        {
            for (unsigned int y = 0; y < 2; ++y)
            {
                for (unsigned int z = 0; z < 2; ++z)
                {
                    const Vector4 pt = inv * Vector4(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f);
                    corners.push_back(Point3((pt / pt.getW()).getXYZ()));
                }
            }
        }

        auto invView = inverse(view);
        drawPoint(Point3(invView.getCol3().getXYZ()), 1.0f, time);

        drawLine(corners[0], corners[1], Color4(0.0f, 0.0f, 1.0f, 1.0f), time);
        drawLine(corners[2], corners[3], Color4(0.0f, 0.0f, 1.0f, 1.0f), time);
        drawLine(corners[4], corners[5], Color4(0.0f, 0.0f, 1.0f, 1.0f), time);
        drawLine(corners[6], corners[7], Color4(0.0f, 0.0f, 1.0f, 1.0f), time);

        drawLine(corners[0], corners[2], Color4(0.0f, 1.0f, 0.0f, 1.0f), time);
        drawLine(corners[1], corners[3], Color4(0.0f, 0.5f, 0.0f, 1.0f), time);
        drawLine(corners[4], corners[6], Color4(0.0f, 1.0f, 0.0f, 1.0f), time);
        drawLine(corners[5], corners[7], Color4(0.0f, 0.5f, 0.0f, 1.0f), time);

        drawLine(corners[0], corners[4], Color4(1.0f, 0.0f, 0.0f, 1.0f), time);
        drawLine(corners[1], corners[5], Color4(0.5f, 0.0f, 0.0f, 1.0f), time);
        drawLine(corners[2], corners[6], Color4(1.0f, 0.0f, 0.0f, 1.0f), time);
        drawLine(corners[3], corners[7], Color4(0.5f, 0.0f, 0.0f, 1.0f), time);
    }
}  // namespace nau
