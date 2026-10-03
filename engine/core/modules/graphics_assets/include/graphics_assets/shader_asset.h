// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <span>

#include "nau/3d/dag_drv3d.h"
#include "nau/3d/dag_drv3dConsts.h"
#include "nau/assets/asset_view.h"
#include "nau/assets/shader_asset_accessor.h"

namespace nau
{
    /**
     */
    class NAU_GRAPHICSASSETS_EXPORT ShaderAssetView : public IAssetView
    {
        NAU_CLASS_(nau::ShaderAssetView, IAssetView)

        using Ptr = nau::Ptr<ShaderAssetView>;

    public:
        static async::Task<nau::Ptr<ShaderAssetView>> createFromAssetAccessor(nau::Ptr<> accessor);
        static PROGRAM makeShaderProgram(std::span<ShaderAssetView::Ptr> shaderAssets, VDECL overrideVDecl = BAD_VDECL);

        const Shader* getShader() const { return &m_shader; }
        VDECL getInputLayout() const { return m_inputLayout; }

    private:
        Shader m_shader;
        VDECL m_inputLayout;
    };
} // namespace nau

namespace nau::shader_globals
{
    NAU_GRAPHICSASSETS_EXPORT void updateTables(const Shader* shader);
} // namespace nau::shaderGlobals
