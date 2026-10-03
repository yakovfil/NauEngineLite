// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <string_view>

#include "graphics_assets/material_asset.h"
#include "graphics_assets/shader_asset.h"
#include "nau/assets/asset_ref.h"

namespace nau::render
{
    // Class that renders full-screen quad for post-effect shaders.
    class NAU_GRAPHICS_EXPORT PostFxRenderer
    {
    public:
        PostFxRenderer(PostFxRenderer&& other);
        PostFxRenderer(const PostFxRenderer& other);
        PostFxRenderer& operator=(PostFxRenderer&& other);
        PostFxRenderer& operator=(const PostFxRenderer& other);
        explicit PostFxRenderer(MaterialAssetView::Ptr material);
        ~PostFxRenderer();

        MaterialAssetView::Ptr getMaterial();

        void render() const;
        void render(std::string_view pipeline) const;

    protected:
        MaterialAssetView::Ptr m_material;

        void drawInternal(std::string_view pipeline) const;
    };

}  // namespace nau::render