// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <nau/3d/dag_drv3d.h>

#include <vector>

#include "graphics_assets/material_asset.h"
#include "nau/assets/asset_view.h"
#include "nau/assets/mesh_asset_accessor.h"
#include "nau/math/dag_bounds3.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    struct SkinnedMeshLod final
    {
        Sbuffer* m_positionsBuffer;
        Sbuffer* m_normalsBuffer;
        Sbuffer* m_tangentsBuffer;
        Sbuffer* m_texcoordsBuffer;
        Sbuffer* m_boneWeightsBuffer;
        Sbuffer* m_boneIndicesBuffer;

        Sbuffer* m_indexBuffer;

        uint32_t m_indexCount;
        uint32_t m_vertexCount;

        nau::math::BBox3 m_localBBox;

        ReloadableAssetView::Ptr m_material;
    };

    class NAU_GRAPHICSASSETS_EXPORT SkinnedMesh : public virtual IRefCounted
    {
        NAU_CLASS_(nau::SkinnedMesh, IRefCounted);
    public:

        using Ptr = nau::Ptr<SkinnedMesh>;

        virtual ~SkinnedMesh() = default;
        SkinnedMesh() = default;

        const nau::SkinnedMeshLod& getLod(uint32_t lodInd) const;
        uint32_t getLodsCount() const;

        inline const nau::math::BSphere3& getLod0BSphere() const
        {
            return m_localBSphere;
        }

    public:
        static async::Task<nau::Ptr<SkinnedMesh>> createFromMeshAccessor(IMeshAssetAccessor& meshAccessor);

    protected:
        nau::math::BSphere3 m_localBSphere;

        std::vector<SkinnedMeshLod> lods;
        std::vector<float> m_lodsScreenSpaceError;
        float cullDistance;
    };

    class NAU_GRAPHICSASSETS_EXPORT SkinnedMeshAssetView : public IAssetView
    {
        NAU_CLASS_(nau::SkinnedMeshAssetView, IAssetView)
    public:
        static async::Task<nau::Ptr<SkinnedMeshAssetView>> createFromAssetAccessor(nau::Ptr<> accessor);

        inline nau::SkinnedMesh::Ptr getMesh()
        {
            return m_skinnedMesh;
        };
    private:
        nau::SkinnedMesh::Ptr m_skinnedMesh;
    };

}  // namespace nau
