// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "graphics_assets/material_asset.h"
#include "instance_group.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/math/dag_frustum.h"
#include "render_list.h"

namespace nau
{
    class RenderView
    {
    public:
        RenderView(std::string_view viewName);
        virtual ~RenderView();

        void addRenderList(RenderList::Ptr list);
        void clearLists();

        void render(const nau::math::Matrix4& vp) const;
        void renderInstanced(const nau::math::Matrix4& vp) const;
        void renderZPrepass(const nau::math::Matrix4& vp, nau::MaterialAssetView* zPrepassMat) const;
        void renderOutlineMask(const nau::math::Matrix4& vp, nau::MaterialAssetView* zPrepassMat) const;

        void updateFrustum(const nau::math::Matrix4& vp);

        void prepareInstanceData();

        const nau::math::NauFrustum& getFrustum() const
        {
            return m_frustum;
        }

        bool containsTag(RenderTag tag);
        void addTag(RenderTag tag);
        void removeTag(RenderTag tag);

        void setUserData(void* userData);
        void* getUserData();

        std::function<bool(const InstanceInfo&)>& getInstanceFilter();
        void setInstanceFilter(std::function<bool(const InstanceInfo&)>& filter);

        std::function<bool(const MaterialAssetView::Ptr)>& getMaterialFilter();
        void setMaterialFilter(std::function<bool(const MaterialAssetView::Ptr)>& filter);

    protected:
        std::string m_viewName;
        nau::math::NauFrustum m_frustum;
        Sbuffer* m_instanceData = nullptr;
        uint32_t m_maxInstancesCount = 0;
        std::vector<RenderList::Ptr> m_lists = {};

        std::function<bool(const InstanceInfo&)> m_instanceFilter;
        std::function<bool(const MaterialAssetView::Ptr)> m_materialFilter;

        RenderTags m_tags;

        void* m_userData = nullptr;
    };

} // namespace nau
