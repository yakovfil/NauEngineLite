// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <fstream>
#include <string>
#include <vector>

#include "material_creator.h"
#include "nau/serialization/json_utils.h"
#include "nau/serialization/serialization.h"
#include "nau/string/string_conv.h"
#include "shader_pack.h"

struct Pipeline
{
    std::string name;
    std::vector<std::string> shaders;
};

struct Arguments
{
    fs::path material;
    fs::path shaderCache;
    std::vector<Pipeline> pipelines;
};

constexpr auto OutKey = "-o";
constexpr auto OutFullKey = "--out";

constexpr auto ShaderCacheKey = "-c";
constexpr auto ShaderCacheFullKey = "-cache";

constexpr auto PipelineKey = "-p";
constexpr auto PipelineFullKey = "--pipeline";

constexpr auto Extension = ".nsbc";

nau::Result<Arguments> parseArguments(int argc, char* argv[]);
void printUsage(std::string_view appName);

int main(int argc, char** argv)
{
    using namespace nau;

    auto args = parseArguments(argc, argv);
    if (args.isError())
    {
        std::cerr << strings::toStringView(args.getError()->getMessage()) << '\n';
        printUsage(argv[0]);
        return EXIT_FAILURE;
    }

    io::IStreamBase::Ptr stream = io::createNativeFileStream(args->shaderCache.string().c_str(), io::AccessMode::Read, io::OpenFileMode::OpenExisting);
    ShaderPack pack(stream);

    MaterialCreator creator;
    auto result = creator.createMaterial(args->material.stem().string().c_str());
    if (result.isError())
    {
        std::cerr << strings::toStringView(result.getError()->getMessage()) << '\n';
        return EXIT_FAILURE;
    }

    for (const auto& [pipelineName, shaderNames] : args->pipelines)
    {
        auto shaders = pack.getShaders(shaderNames);
        if (shaders.isError())
        {
            std::cerr << strings::toStringView(shaders.getError()->getMessage()) << '\n';
            return EXIT_FAILURE;
        }

        result = creator.addPipeline(pipelineName, args->shaderCache.filename().string().c_str(), *shaders);
        if (result.isError())
        {
            std::cerr << strings::toStringView(result.getError()->getMessage()) << '\n';
            return EXIT_FAILURE;
        }
    }

    auto mat = creator.getResult();
    if (mat.isError())
    {
        std::cerr << strings::toStringView(mat.getError()->getMessage()) << '\n';
        return EXIT_FAILURE;
    }

    std::ofstream file(args->material.string().c_str());
    if (!file)
    {
        std::cerr << "Cannot create material file: " << args->material.string();
        return EXIT_FAILURE;
    }

    const std::u8string json = serialization::JsonUtils::stringify(*mat);
    file << strings::toStringView(json).data();
    if (!file)
    {
        std::cerr << "Cannot write material file: " << args->material.string();
        return EXIT_FAILURE;
    }

    std::cout << std::format("Material successfully created: {}\n", args->material.string());

    return EXIT_SUCCESS;
}

nau::Result<Arguments> parseArguments(int argc, char* argv[])
{
    Arguments args;

    auto hasOut = false;
    auto hasCache = false;

    for (auto i = 1; i < argc; ++i)
    {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            printUsage(argv[0]);
            std::exit(EXIT_SUCCESS);
        }

        if (arg == OutKey || arg == OutFullKey)
        {
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                args.material = argv[++i];
                hasOut = true;
            }
            else
            {
                return NauMakeError("Missing value for {}/{}", OutKey, OutFullKey);
            }
        }
        else if (arg == ShaderCacheKey || arg == ShaderCacheFullKey)
        {
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                args.shaderCache = argv[++i];
                hasCache = true;
            }
            else
            {
                return NauMakeError("Missing value for {}/{}", ShaderCacheKey, ShaderCacheFullKey);
            }
        }
        else if (arg == "-p" || arg == "--pipeline")
        {
            if (i + 1 < argc && argv[i + 1][0] != '-')
            {
                std::string pipelineName = argv[++i];

                std::vector<std::string> shaders;
                while (i + 1 < argc && argv[i + 1][0] != '-')
                {
                    shaders.emplace_back(argv[++i]);
                }

                if (shaders.empty())
                {
                    return NauMakeError("Missing shader names for pipeline: {}", pipelineName);
                }

                args.pipelines.emplace_back(Pipeline{pipelineName, shaders});
            }
            else
            {
                return NauMakeError("Missing pipeline name after {}/{}", PipelineKey, PipelineFullKey);
            }
        }
        else
        {
            return NauMakeError("Unknown argument: {}", arg);
        }
    }

    if (!hasOut)
    {
        return NauMakeError("Missing required argument: {}/{}", OutKey, OutFullKey);
    }
    if (!hasCache)
    {
        return NauMakeError("Missing required argument: {}/{}", ShaderCacheKey, ShaderCacheFullKey);
    }
    if (args.pipelines.empty())
    {
        return NauMakeError("At least one pipeline must be specified with {}/{}", PipelineKey, PipelineFullKey);
    }

    return args;
}

void printUsage(std::string_view appName)
{
    const fs::path fullName(appName);

    std::cout << std::format(
        "Usage: {} -o <material_file> -c <shader_cache_path> -p <pipeline_name> <shader1> <shader2> ... [-p <pipeline_name> <shader1> <shader2> ...]\n",
        fullName.filename().string()
        );

    std::cout << "\nOptions:\n";
    std::cout << "  -h, --help             Display this help message and exit.\n";
    std::cout << "  -o, --out              Material file to be created (required).\n";
    std::cout << "  -c, --cache            Path to the shader cache (required).\n";
    std::cout << "  -p, --pipeline         Specify a pipeline name followed by a list of shader names (required, can be repeated).\n";
}
