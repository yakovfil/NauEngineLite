// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "graphics_assets/static_mesh_asset.h"

#include <functional>
#include <optional>

#include "nau/assets/mesh_asset_accessor.h"

namespace nau
{
    async::Task<nau::Ptr<StaticMeshAssetView>> StaticMeshAssetView::createFromAssetAccessor(nau::Ptr<> accessor)
    {
        using namespace nau::async;

        NAU_ASSERT(accessor);

        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault());

        auto& meshAccessor = accessor->as<IMeshAssetAccessor&>();
        auto meshAssetView = rtti::createInstance<StaticMeshAssetView>();

        meshAssetView->m_mesh = co_await StaticMesh::createFromStaticMeshAccessor(meshAccessor);

        co_return meshAssetView;
    }

    Sbuffer* StaticMeshAssetView::getPositionsBuffer() const
    {
        return m_mesh->getLod(0).m_positionsBuffer;
    }

    Sbuffer* StaticMeshAssetView::getNormalsBuffer() const
    {
        return m_mesh->getLod(0).m_normalsBuffer;
    }

    Sbuffer* StaticMeshAssetView::getTangentsBuffer() const
    {
        return m_mesh->getLod(0).m_tangentsBuffer;
    }

    Sbuffer* StaticMeshAssetView::getTexcoordsBuffer() const
    {
        return m_mesh->getLod(0).m_texCoordsBuffer;
    }

    Sbuffer* StaticMeshAssetView::getIndexBuffer() const
    {
        return m_mesh->getLod(0).m_indexBuffer;
    }

    unsigned StaticMeshAssetView::getIndexCount() const
    {
        return m_mesh->getLod(0).m_indexCount;
    }

    unsigned StaticMeshAssetView::getVertexCount() const
    {
        return m_mesh->getLod(0).m_vertexCount;
    }

    void StaticMeshAssetView::enumerateMeshTriangles(
        Functor<void(const nau::math::vec3&, const nau::math::vec3&, const nau::math::vec3&)> sink) const
    {
        Sbuffer* positionBuffer = getPositionsBuffer();
        Sbuffer* indexBuffer = getIndexBuffer();

        const unsigned vertexCount = getVertexCount();
        const auto indexCount = getIndexCount();

        const size_t posBufferSize = vertexCount * sizeof(float[3]);

        std::byte* positionMemory = nullptr;
        positionBuffer->lock(0, posBufferSize, reinterpret_cast<void**>(&positionMemory), VBLOCK_READONLY);

        // todo: NAU-1797 Remove hardcode, support 32 bit indices geometry
        // todo: NAU-1797 Stop readback, get geometry data from asset instead
        const size_t indexBufferSize = indexCount * sizeof(uint16_t); 
        std::byte* indexMemory = nullptr;
        indexBuffer->lock(0, indexBufferSize, reinterpret_cast<void**>(&indexMemory), VBLOCK_READONLY);

        auto indices = reinterpret_cast<uint16_t const*>(indexMemory);
        auto points = reinterpret_cast<float const*>(positionMemory);

        const std::size_t trVertCount = 3;
        const auto readPoint3 =
            [&posBufferSize, &trVertCount, points](uint16_t const* indexPtr) -> std::optional<nau::math::vec3>
        {
            unsigned index = *indexPtr;
            if (index + trVertCount - 1 >= posBufferSize) 
            {
                return std::nullopt;
            }

            return nau::math::vec3{
                    points[trVertCount * index + 0],
                    points[trVertCount * index + 1],
                    points[trVertCount * index + 2]
            };
        };

        auto begin = indices;
        auto end = indices + indexCount;

        for (auto it = begin; it != std::prev(end, trVertCount); it += trVertCount)
        {
            auto p1 = readPoint3(it);
            auto p2 = readPoint3(std::next(it));
            auto p3 = readPoint3(std::next(std::next(it)));

            if (!p1 || !p2 || !p3)
            {
                break;
            }

            std::invoke(sink, *p1, *p2, *p3);
        }
    }

}  // namespace nau
