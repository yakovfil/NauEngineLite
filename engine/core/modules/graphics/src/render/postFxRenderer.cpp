// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <string_view>

#include "graphics_assets/material_asset.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/graphics/core_graphics.h"
#include "nau/render/dag_postFxRenderer.h"
#include "nau/shaders/dag_shaderCommon.h"

namespace nau::render
{

    PostFxRenderer::PostFxRenderer(PostFxRenderer&& other) = default;
    PostFxRenderer& PostFxRenderer::operator=(PostFxRenderer&& other) = default;
    PostFxRenderer::PostFxRenderer(const PostFxRenderer& other) = default;
    PostFxRenderer& PostFxRenderer::operator=(const PostFxRenderer& other) = default;
    PostFxRenderer::~PostFxRenderer() = default;

    PostFxRenderer::PostFxRenderer(MaterialAssetView::Ptr material) :
        m_material(material)
    {
    }

    void PostFxRenderer::drawInternal(std::string_view pipeline) const
    {
        NAU_ASSERT(getServiceProvider().has<nau::ICoreGraphics>());

        if (pipeline.empty())
        {
            m_material->bind();
        }
        else
        {
            m_material->bindPipeline(pipeline);
        }

        d3d::setvsrc(0, nullptr, 0);
        d3d::setind(nullptr);

        d3d::draw(PRIM_TRISTRIP, 0, 2);  // Draw quad
    }

    void PostFxRenderer::render() const
    {
        drawInternal("");
    }

    void PostFxRenderer::render(std::string_view pipeline) const
    {
        drawInternal(pipeline);
    }

    MaterialAssetView::Ptr PostFxRenderer::getMaterial()
    {
        return m_material;
    }

}  // namespace nau::render
