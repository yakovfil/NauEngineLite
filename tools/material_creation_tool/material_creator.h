// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "nau/assets/material.h"

namespace nau
{
    class MaterialCreator final
    {
    public:
        Result<> createMaterial(std::string_view name);
        Result<> addPipeline(std::string_view name, std::string_view shaderCacheFilename, const std::vector<Shader>& shaders);

        Result<Material> getResult() const;

        void clear();

    private:
        void processCbuffer(MaterialPipeline& pipeline, const ShaderInputBindDescription& bind);
        void processTexture(MaterialPipeline& pipeline, const ShaderInputBindDescription& bind);
        void processSampler(MaterialPipeline& pipeline, const ShaderInputBindDescription& bind);

        std::optional<Material> m_material = std::nullopt;
    };
} // namespace nau
