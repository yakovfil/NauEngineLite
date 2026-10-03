// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "shader_pack.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

#include "nau/io/nau_container.h"
#include "nau/io/stream.h"
#include "nau/serialization/runtime_value_builder.h"

namespace nau
{
    ShaderPack::ShaderPack(io::IStreamReader::Ptr stream)
        : m_stream(stream)
    {
        using namespace nau::io;

        auto [packHeader, blobStartOffset] = *io::readContainerHeader(m_stream);

        RuntimeValue::assign(makeValueRef(m_shadersPackData), packHeader).ignore();
    }

    Result<Shader> ShaderPack::getShader(std::string_view name)
    {
        Shader* shader = findShader(name);

        return shader != nullptr
            ? Result<Shader>{*shader}
            : NauMakeError("Shader not found");
    }

    Result<std::vector<Shader>> ShaderPack::getShaders(const std::vector<std::string>& names)
    {
        std::vector<Shader> shaders;
        shaders.reserve(names.size());

        for (const auto& name : names)
        {
            Shader* shader = findShader(name);
            if (shader == nullptr)
            {
                return NauMakeError("Shader not found");
            }

            shaders.emplace_back(*shader);
        }

        return {std::move(shaders)};
    }

    Shader* ShaderPack::findShader(std::string_view shaderName)
    {
        auto& shaders = m_shadersPackData.shaders;

        if (shaderName.empty())
        {
            // container's default content
            if (shaders.size() != 1)
            {
                std::cerr << "Requesting default shader, but there is more shaders" << std::flush;
            }

            return !shaders.empty() ? &shaders.front() : nullptr;
        }

        auto iter = std::find_if(shaders.begin(), shaders.end(), [shaderName](const nau::Shader& shader)
        {
            return shader.name == shaderName;
        });

        return iter != shaders.end() ? &(*iter) : nullptr;
    }
} // namespace nau
