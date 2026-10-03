// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <set>
#include <string_view>
#include <vector>

#include "graphics_assets/material_asset.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/platform/windows/utils/uid.h"

namespace nau
{
    using RenderTag = size_t;
    using RenderTags = std::set<RenderTag>;

    struct RenderEntity
    {
        VECTORMATH_ALIGNED_TYPE_PRE struct InstanceData
        {
            nau::math::Matrix4 worldMatrix;
            nau::math::Matrix4 normalMatrix;
            nau::Uid uid;
            uint32_t isHighlighted;
        };

        struct ConstBufferStructData
        {
            uint32_t size;
            void* dataPtr;
        };

        Sbuffer* positionBuffer = nullptr;
        Sbuffer* normalsBuffer = nullptr;
        Sbuffer* texcoordsBuffer = nullptr;
        Sbuffer* tangentsBuffer = nullptr;

        Sbuffer* boneWeightsBuffer = nullptr;
        Sbuffer* boneIndicesBuffer = nullptr;

        Sbuffer* indexBuffer = nullptr;

        bool instancingSupported = true;

        uint32_t startInstance;
        uint32_t instancesCount;
        std::vector<InstanceData> instanceData;

        nau::Ptr<nau::MaterialAssetView> material;

        uint32_t startIndex;
        uint32_t endIndex;

        RenderTags tags;

        nau::math::Matrix4 worldTransform;
        std::map<std::string_view, ConstBufferStructData> cbStructsData;

        void render(nau::math::Matrix4 viewProj) const;
        void renderInstanced(nau::math::Matrix4 viewProj, Sbuffer* instanceData) const;

        void renderZPrepass(const math::Matrix4& viewProj, MaterialAssetView* zPrepassMat) const;
        void renderZPrepassInstanced(const math::Matrix4& viewProj, MaterialAssetView* zPrepassMat) const;
    private:
        void prepareZPrepass(std::string_view pipeline, const math::Matrix4& viewProj, MaterialAssetView* zPrepassMat) const;
    };

} // namespace nau
