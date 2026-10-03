// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <functional>
#include <map>
#include <vector>

#include "graphics_assets/material_asset.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/math/dag_bounds3.h"
#include "render_entity.h"
#include "render_list.h"

namespace nau
{
    using InstanceID = uint64_t;

    struct MaterialOverrideInfo
    {
        uint64_t lodSlot;
        ReloadableAssetView::Ptr material;
    };


    struct InstanceInfo
    {
        InstanceID id;

        nau::math::Matrix4 worldMatrix;
        nau::math::BSphere3 worldSphere;
        nau::math::BSphere3 localSphere;

        std::map<uint64_t, MaterialOverrideInfo> overrideInfo;
        bool toDelete = false;

        // RenderParameters
        RenderTags tags;
        bool isVisible = true;
        bool isCastShadow = true;
        bool isHighlighted = false;

        nau::Uid uid;
    };


    class IInstanceGroup
    {
    public:
        virtual size_t getInstancesCount() const = 0;
        virtual InstanceInfo& getInstance(InstanceID instID) = 0;
        virtual void removeInstance(InstanceID instID) = 0;
        virtual bool contains(InstanceID instID) const = 0;
        virtual RenderEntity createRenderEntity() = 0;
        virtual RenderList::Ptr createRenderList(const nau::math::Vector3& viewerPosition,
                                                 std::function<bool(const InstanceInfo&)>& filterFunc,
                                                 std::function<bool(const nau::MaterialAssetView::Ptr)>& materialFilter) = 0;

    protected:
        RenderTags tags;
    };

} // namespace nau
