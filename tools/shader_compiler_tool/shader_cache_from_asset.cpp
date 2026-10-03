// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "shader_cache_from_asset.h"

#include <windows.h>

#include <codecvt>
#include <fstream>
#include <locale>
#include <string>
#include <vector>

#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/io/file_system.h"
#include "nau/io/nau_container.h"
#include "nau/memory/bytes_buffer.h"
#include "nau/platform/windows/utils/uid.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/usd_meta_tools/usd_meta_manager.h"

namespace nau
{
    namespace
    {
        constexpr auto DefaultInputLayoutName = "DefaultInputLayout";
        constexpr auto ShaderType = "shader";

        constexpr const char* Targets[] = {
            "vs",
            "ps",
            "gs",
            "hs",
            "ds",
            "cs",
        };
        static_assert(
            std::size(Targets) == static_cast<size_t>(ShaderTarget::Count),
            "Targets array size does not match the number of ShaderTarget enum values"
            );

        Result<ShaderTarget> stringToShaderTarget(std::string_view target)
        {
            const auto* it = std::ranges::find(Targets, target);
            if (it != std::end(Targets))
            {
                const ptrdiff_t index = std::distance(std::begin(Targets), it);
                return static_cast<ShaderTarget>(index);
            }

            return NauMakeError("Invalid shader target: {}", target);
        }

        struct ShaderBytecodeEntry
        {
            std::string shaderName;
            size_t blobOffset;
            size_t blobSize;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(shaderName),
                CLASS_FIELD(blobOffset),
                CLASS_FIELD(blobSize)
            )
        };

        struct ShaderPackContainerData
        {
            std::vector<Shader> shaders;
            std::vector<ShaderBytecodeEntry> byteCode;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(shaders),
                CLASS_FIELD(byteCode)
            )
        };

        Result<> writeShadersPack(const io::IStreamWriter::Ptr& outStream, std::vector<Shader>&& shaders)
        {
            ShaderPackContainerData containerData;
            containerData.shaders = std::move(shaders);

            auto bytecodeStream = io::createMemoryStream(BytesBuffer{});

            for (const Shader& shader : containerData.shaders)
            {
                ShaderBytecodeEntry& entry = containerData.byteCode.emplace_back();
                entry.shaderName = shader.name;
                entry.blobOffset = bytecodeStream->getPosition();
                entry.blobSize = shader.bytecode.size();

                bytecodeStream->write(shader.bytecode.data(), shader.bytecode.size()).ignore();
            }

            bytecodeStream->setPosition(io::OffsetOrigin::Begin, 0);

            io::writeContainerHeader(outStream, "nau-shader-pack", makeValueRef(containerData));
            NauCheckResult(io::copyStream(outStream->as<io::IStreamWriter&>(), bytecodeStream->as<io::IStreamReader&>()));

            return ResultSuccess;
        }

        Result<> validatePaths(const fs::path& metafilesPath)
        {
            if (!fs::exists(metafilesPath))
            {
                return NauMakeError("File or directory does not exist: {}", metafilesPath.string());
            }

            if (fs::is_directory(metafilesPath) ||fs::is_regular_file(metafilesPath))
            {
                return NauMakeError("Metafiles must be directories or files:\n{}", metafilesPath.string());
            }

            return ResultSuccess;
        }
    } // anonymous namespace

    Result<> ShaderCacheBuilderFromAsset::makeCache(StreamFactory streamFactory, const Arguments& args)
    {
        auto shaderInfos = collectShaderInfo(args.metafilesPath);
        if (shaderInfos.isError())
        {
            return NauMakeError(shaderInfos.getError()->getMessage());
        }

        auto result = compileShaders(*shaderInfos, args);
        NauCheckResult(result);

        io::IStreamWriter::Ptr outStream = streamFactory(args.shaderCacheName);
        NAU_FATAL(outStream);

        return writeShadersPack(outStream, *std::move(result));
    }

    Result<> ShaderCacheBuilderFromAsset::makeCacheFiles(StreamFactory streamFactory, const Arguments& args)
    {
        auto shaderInfos = collectShaderInfo(args.metafilesPath);
        if (shaderInfos.isError())
        {
            return NauMakeError(shaderInfos.getError()->getMessage());
        }

        auto result = compileShaders(*shaderInfos, args);
        NauCheckResult(result);

        std::vector<Shader> shaders;

        for (Shader& shader : *result)
        {
            const std::string_view shaderName{ shader.name.data(), shader.name.size() };
            io::IStreamWriter::Ptr outStream = streamFactory(shaderName);
            NAU_FATAL(outStream);

            shaders.emplace_back(std::move(shader));
            NauCheckResult(writeShadersPack(outStream, std::move(shaders)));

            shaders.clear();
        }

        return {};
    }

    Result<std::vector<fs::path>> ShaderCacheBuilderFromAsset::collectShaderInfo(const fs::path& metafilesPath)
    {
        auto validation = validatePaths(metafilesPath);
        NauCheckResult(validation);

        std::vector<fs::path> shaderInfos;

        if (fs::is_regular_file(metafilesPath))
        {
            shaderInfos.emplace_back(metafilesPath);
        }
        else
        {
            auto metafiles = collectFiles(metafilesPath, ".usda");
            NauCheckResult(metafiles);
            shaderInfos = *metafiles;
        }

        return shaderInfos;
    }


    nau::Result<std::vector<fs::path>> ShaderCacheBuilderFromAsset::collectFiles(const fs::path& directory, std::string_view extension)
    {
        if (extension.empty() || !extension.starts_with('.'))
        {
            return NauMakeError("This is not a file extension: {}", extension);
        }

        std::vector<fs::path> files;

        for (const auto& entry : fs::recursive_directory_iterator(directory))
        {
            if (entry.is_regular_file() && entry.path().extension() == extension)
            {
                files.emplace_back(entry);
            }
        }

        if (files.empty())
        {
            return NauMakeError("No files collected: {}", directory.string());
        }

        return files;
    }

    Result<std::vector<Shader>> ShaderCacheBuilderFromAsset::compileShaders(const std::vector<fs::path>& shaderInfos, const Arguments& args)
    {
        std::vector<std::wstring> includes;
        includes.reserve(args.includeDirs.size());

        for (const auto& dir : args.includeDirs)
        {
            includes.emplace_back(dir.c_str());
        }

        std::vector<Shader> shaders;
        ShaderCompiler compiler;
        for (const auto& metafile : shaderInfos)
        {
            auto meta = UsdMetaManager::instance().getInfo(metafile.string());
            nau::UsdMetaInfo* metaInfo = nullptr;

            for (auto it : meta)
            {
                if (it.type == ShaderType)
                    metaInfo = &it;

                auto result = compileShader(&compiler, it, includes, args.debugOutputDir, args.embedDebugInfo);
                NauCheckResult(result);

                shaders.reserve(shaders.size() + result->size());
                std::move(result->begin(), result->end(), std::back_inserter(shaders));
            }
        }

        return shaders;
    }

    nau::Result<std::vector<Shader>> ShaderCacheBuilderFromAsset::compileShader(
        ShaderCompiler* compiler, 
        const nau::UsdMetaInfo& metaInfo,
        const std::vector<std::wstring>& includeDirs, 
        const std::optional<fs::path>& pdbDir, 
        bool needEmbedDebug
    )
    {
        auto& meta = *metaInfo.getExtraInfoAs<ExtraInfoShader>();
        fs::path filename = meta.path;
        compiler->reset();

        auto result = compiler->loadFile(filename);
        if (result.isError())
        {
            return NauMakeError(result.getError()->getMessage());
        }

        std::vector<Shader> shaders;
        for (const auto& it : meta.configs)
        {
            auto& config = it.second;

            std::string shaderName = filename.stem().string();
            shaderName += "." + it.first + ".";
            shaderName += config.target;

            std::string ep = config.entryPoint;
            std::ranges::transform(ep, ep.begin(), [](unsigned char c) { return std::tolower(c); });
            shaderName += "." + ep;

            std::optional<fs::path> pdbFilename = std::nullopt;
            if (pdbDir.has_value())
            {
                const std::string pdbName = shaderName + ".pdb";
                pdbFilename = (*pdbDir / pdbName).string();
            }
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::vector<std::wstring> defines;
            std::ranges::transform(config.defines, defines.begin(), [&converter](std::string c) { return converter.from_bytes(c); });

            NauCheckResult(compiler->compile(*stringToShaderTarget(config.target), config.entryPoint, defines, includeDirs, pdbFilename, needEmbedDebug));

            Shader& shader = shaders.emplace_back(*compiler->getResult());
            shader.name.assign(shaderName.data(), shaderName.size());

            std::string layout = DefaultInputLayoutName;
            if (!config.inputLayout.empty() && meta.layouts.find(config.inputLayout) != meta.layouts.end())
            {
                layout = config.inputLayout;
            }
            auto layoutIt = meta.layouts.find(layout);
            if (layoutIt != meta.layouts.end())
            {
                std::vector<VertexShaderDeclaration> vsd;
                vsd.resize(layoutIt->second.items.size());

                for (int i = 0; auto layoutItem : layoutIt->second.items)
                {
                    VertexShaderDeclaration& dest = vsd[i++];
                    dest.stream = layoutIt->second.stream.c_str();
                    dest.number = layoutItem.second.bufferIndex;
                    dest.vsdReg.resize(1);
                    dest.vsdReg[0].semanticName = layoutItem.first.c_str();
                    dest.vsdReg[0].type = layoutItem.second.type.c_str();
                }
                shader.vsd.assign(vsd.begin(), vsd.end());
            }
            else
            {
                return NauMakeError("DefaultInputLayout does not exist: {}", metaInfo.assetPath);
            }
        }
        return shaders;
    }

} // namespace nau
