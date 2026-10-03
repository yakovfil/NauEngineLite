// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <vector>

#include "nau/assets/shader_asset_accessor.h"
#include "shader_cache.h"
#include "shader_compiler.h"

namespace fs = std::filesystem;

namespace nau
{
    class ShaderCacheBuilder final : public IShaderCache
    {
    public:

        Result<> makeCache(StreamFactory streamFactory, const Arguments& args) override;
        Result<> makeCacheFiles(StreamFactory streamFactory, const Arguments& args) override;

    private:
        struct ShaderInfo
        {
            fs::path srcFile;
            fs::path metaFile;
        };

        struct CompileConfig
        {
            std::string entry;
            ShaderTarget stage;
        };

        struct ShaderPermutation
        {
            std::string name;
            std::vector<std::wstring> defines;
        };

        struct ShaderMeta
        {
            std::vector<CompileConfig> configs;
            std::vector<ShaderPermutation> permutations;
            std::vector<VertexShaderDeclaration> vsd;
        };

        Result<std::vector<ShaderInfo>> collectShaderInfo(const fs::path& shadersPath, const fs::path& metafilesPath);
        Result<std::vector<fs::path>> collectFiles(const fs::path& directory, std::string_view extension);

        Result<std::vector<Shader>> compileShaders(const std::vector<ShaderInfo>& shaderInfos, const Arguments& args);

        Result<std::vector<Shader>> compileShader(
            ShaderCompiler* compiler,
            const fs::path& filename,
            const ShaderMeta& meta,
            const std::vector<std::wstring>& includeDirs,
            const std::optional<fs::path>& pdbDir,
            bool needEmbedDebug);

        Result<ShaderMeta> getShaderMeta(const fs::path& filename);
    };
} // namespace nau
