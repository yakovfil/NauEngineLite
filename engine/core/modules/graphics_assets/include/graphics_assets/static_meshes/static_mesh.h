// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <vector>

#include "graphics_assets/material_asset.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/3d/dag_resPtr.h"
#include "nau/assets/asset_ref.h"
#include "nau/assets/mesh_asset_accessor.h"
#include "nau/async/task_base.h"
#include "nau/math/dag_bounds3.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    struct StaticMeshDescriptor
    {
        StaticMeshAssetRef staticMeshRef;
    };


    struct MaterialSlot
    {
        uint32_t m_startIndex;
        uint32_t m_endIndex;

        ReloadableAssetView::Ptr m_material;
    };


    struct StaticMeshLod
    {
        Sbuffer* m_positionsBuffer; // TODO: change to ResPtr's
        Sbuffer* m_normalsBuffer;
        Sbuffer* m_tangentsBuffer;
        Sbuffer* m_texCoordsBuffer;

        Sbuffer* m_indexBuffer;

        uint32_t m_indexCount;
        uint32_t m_vertexCount;

        nau::math::BBox3 m_localBBox;

        std::vector<MaterialSlot> m_materialSlots;
    };


    class NAU_GRAPHICSASSETS_EXPORT StaticMesh : public virtual IRefCounted
    {
        NAU_CLASS_(nau::StaticMesh, IRefCounted);
    public:

        using Ptr = nau::Ptr<StaticMesh>;

        virtual ~StaticMesh() = default;
        StaticMesh();

        const nau::StaticMeshLod& getLod(uint32_t lodInd) const;
        uint32_t getLodsCount() const;

        inline const nau::math::BSphere3& getLod0BSphere() const
        {
            return m_localBSphere;
        }

    public:
        static async::Task<nau::Ptr<StaticMesh>> createFromStaticMeshAccessor(IMeshAssetAccessor& accessor);

        static bool createFromGeneratedData();

    protected:
        StaticMeshDescriptor m_meshDescriptor;

        nau::math::BSphere3 m_localBSphere;

        std::vector<StaticMeshLod> lods;
        std::vector<float> m_lodsScreenSpaceError;
        float cullDistance;
    };
}
