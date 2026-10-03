// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "material_creator.h"

#include <optional>
#include <string_view>
#include <vector>

#include "nau/serialization/runtime_value_builder.h"

namespace nau
{
    Result<> MaterialCreator::createMaterial(std::string_view name)
    {
        if (m_material.has_value())
        {
            return NauMakeError("Material already created: {}\nCannot create a new material: {}", m_material->name, name);
        }

        m_material = Material{
            .name = name.data(),
            .master = std::nullopt,
            .pipelines = {}};

        return ResultSuccess;
    }

    Result<> MaterialCreator::addPipeline(std::string_view name, std::string_view shaderCacheFilename, const std::vector<Shader>& shaders)
    {
        if (!m_material.has_value())
        {
            return NauMakeError("Material not created");
        }

        // We create a single default pipeline with all shaders passed to the tool.
        auto& pipeline = m_material->pipelines[name.data()];
        pipeline.shaders.reserve(shaders.size());

        for (const auto& shader : shaders)
        {
            // Currently, it is impossible to calculate the relative path for shaders
            // because paths for built-in and custom shaders have not been established yet.
            // For now, the path will be hardcoded.
            // TODO: We need a solution in the io::FsPath module to obtain these paths and correctly reference assets, even when used in external tools.
            // TODO: It might be useful to implement a system similar to environment variables, which would have default values but could be customized by the user.
            // TODO: For example, ${BUILTIN_SHADERS} and ${USER_SHADERS}.
            pipeline.shaders.push_back(std::format("file:/content/shaders/cache/{}+[{}]", shaderCacheFilename.data(), shader.name.c_str()).c_str());

            for (const auto& bind : shader.reflection.inputBinds)
            {
                switch (bind.type)
                {
                    case ShaderInputType::CBuffer:
                        processCbuffer(pipeline, bind);
                        break;
                    case ShaderInputType::Texture:
                        processTexture(pipeline, bind);
                        break;
                    case ShaderInputType::Sampler:
                        processSampler(pipeline, bind);
                        break;
                    case ShaderInputType::Structured:
                    case ShaderInputType::UavRwTyped:
                    case ShaderInputType::UavRwStructured:
                    case ShaderInputType::UavRwStructuredWithCounter:
                        // We don't use these in the material asset, so skip them.
                        break;

                    default:
                        NAU_FAILURE_ALWAYS("Not implemented");
                }
            }
        }

        return ResultSuccess;
    }

    Result<Material> MaterialCreator::getResult() const
    {
        return m_material.has_value()
            ? Result<Material>{*m_material}
            : NauMakeError("Material not created");
    }

    void MaterialCreator::clear()
    {
        m_material.reset();
    }

    void MaterialCreator::processCbuffer(MaterialPipeline& pipeline, const ShaderInputBindDescription& bind)
    {
        if (bind.bindPoint == 0)
        {
            return;
        }

        for (const auto& var : bind.bufferDesc.variables)
        {
            switch (var.type.svc)
            {
                case ShaderVariableClass::Scalar:
                {
                    switch (var.type.svt)
                    {
                        case ShaderVariableType::Int:
                        {
                            pipeline.properties[var.name] = makeValueCopy(0);
                            break;
                        }
                        case ShaderVariableType::Uint:
                        {
                            pipeline.properties[var.name] = makeValueCopy(0U);
                            break;
                        }
                        case ShaderVariableType::Float:
                        {
                            pipeline.properties[var.name] = makeValueCopy(0.0F);
                            break;
                        }
                        default:
                            NAU_FAILURE_ALWAYS("Not implemented");
                    }
                    break;
                }
                case ShaderVariableClass::Vector:
                {
                    switch (var.type.svt)
                    {
                        case ShaderVariableType::Float:
                        {
                            switch (var.type.columns)
                            {
                                case 2:
                                {
                                    pipeline.properties[var.name] = makeValueCopy(std::array{0.0F, 0.0F});
                                    break;
                                }
                                case 3:
                                {
                                    pipeline.properties[var.name] = makeValueCopy(std::array{0.0F, 0.0F, 0.0F});
                                    break;
                                }
                                case 4:
                                {
                                    pipeline.properties[var.name] = makeValueCopy(std::array{0.0F, 0.0F, 0.0F, 0.0F});
                                    break;
                                }
                                default:
                                    NAU_FAILURE_ALWAYS("Not implemented");
                            }
                            break;
                        }
                        default:
                            NAU_FAILURE_ALWAYS("Not implemented");
                    }
                    break;
                }
                case ShaderVariableClass::MatrixColumns:
                {
                    NAU_ASSERT(var.type.columns == var.type.rows);

                    switch (var.type.svt)
                    {
                        case ShaderVariableType::Float:
                        {
                            switch (var.type.columns)
                            {
                                case 3:
                                {
                                    static constexpr std::array data = {
                                        1.0F, 0.0F, 0.0F,
                                        0.0F, 1.0F, 0.0F,
                                        0.0F, 0.0F, 1.0F,
                                    };

                                    pipeline.properties[var.name] = makeValueCopy(data);
                                    break;
                                }
                                case 4:
                                {
                                    static constexpr std::array data = {
                                        1.0F, 0.0F, 0.0F, 0.0F,
                                        0.0F, 1.0F, 0.0F, 0.0F,
                                        0.0F, 0.0F, 1.0F, 0.0F,
                                        0.0F, 0.0F, 0.0F, 1.0F,
                                    };

                                    pipeline.properties[var.name] = makeValueCopy(data);
                                    break;
                                }
                                default:
                                    NAU_FAILURE_ALWAYS("Not implemented");
                            }
                            break;
                        }
                        default:
                            NAU_FAILURE_ALWAYS("Not implemented");
                    }
                    break;
                }
                default:
                    NAU_FAILURE_ALWAYS("Not implemented");
            }
        }
    }

    void MaterialCreator::processTexture(MaterialPipeline& pipeline, const ShaderInputBindDescription& bind)
    {
        // Add only textures (Texture1D, Texture2D, etc.) to the material asset.
        if (bind.dimension != SrvDimension::Buffer)
        {
            pipeline.properties[bind.name] = makeValueCopy(std::string{"file:/content/textures/default.jpg"});
        }
    }

    void MaterialCreator::processSampler(MaterialPipeline& pipeline, const ShaderInputBindDescription& bind)
    {
        pipeline.properties[bind.name] = makeValueCopy(0);
    }
} // namespace nau
