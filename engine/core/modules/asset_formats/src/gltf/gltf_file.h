// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "nau/io/file_system.h"
#include "nau/meta/class_info.h"
#include "nau/serialization/runtime_value.h"
#include "nau/string/string_utils.h"
#include "nau/utils/result.h"

namespace nau
{
    struct GltfHeader
    {
        std::string generator;
        std::string version;
        RuntimeValue::Ptr extras;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(generator),
            CLASS_FIELD(version),
            CLASS_FIELD(extras))

#pragma endregion
    };

    using Float3 = std::array<float, 3>;
    using Float4 = std::array<float, 4>;

    struct GltfNodeBase
    {
        std::string name;
        std::vector<unsigned> children;
        Float3 translation = {0.f, 0.f, 0.f};
        Float3 scale = {1.f, 1.f, 1.f};
        Float4 rotation = {0.f, 0.f, 0.f, 1.f};

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(children),
            CLASS_FIELD(translation),
            CLASS_FIELD(scale),
            CLASS_FIELD(rotation))

#pragma endregion
    };

    struct GltfSceneData
    {
        std::string name;
        std::vector<unsigned> nodes;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(nodes), )

#pragma endregion
    };

    struct GltfMeshData
    {
        struct MeshPrimitive
        {
            std::map<std::string, unsigned> attributes;
            unsigned indices;
            std::optional<unsigned> material;

#pragma region Class Info

            NAU_CLASS_FIELDS(
                CLASS_FIELD(attributes),
                CLASS_FIELD(indices),
                CLASS_FIELD(material))

#pragma endregion
        };

        std::string name;
        std::vector<MeshPrimitive> primitives;
        std::map<std::string, RuntimeValue::Ptr> extras;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(primitives),
            CLASS_FIELD(extras))

#pragma endregion
    };

    struct GltfCameraData
    {
        struct GltfPerspective
        {
            float aspectRatio = 1.0f;
            float yFov = 0.35f;
            float zNear = 0.1f;
            float zFar = 1000.f;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(aspectRatio),
                CLASS_FIELD(yFov),
                CLASS_FIELD(zNear),
                CLASS_FIELD(zFar))
        };

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(type),
            CLASS_FIELD(perspective))

        std::string name;
        std::string type;
        std::optional<GltfPerspective> perspective;
    };

    struct GltfAccessor
    {
        using FArray3 = std::array<float, 3>;

        unsigned bufferView;
        unsigned componentType;
        unsigned count;
        std::string type;
        std::optional<FArray3> max;
        std::optional<FArray3> min;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(bufferView),
            CLASS_FIELD(componentType),
            CLASS_FIELD(count),
            CLASS_FIELD(type))

#pragma endregion
    };

    struct GltfBufferView
    {
        unsigned buffer;
        unsigned byteLength;
        unsigned byteOffset;
        std::optional<unsigned> byteStride;
        unsigned target;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(buffer),
            CLASS_FIELD(byteLength),
            CLASS_FIELD(byteOffset),
            CLASS_FIELD(byteStride),
            CLASS_FIELD(target))

#pragma endregion
    };

    struct GltfBuffer
    {
        unsigned byteLength;
        std::string uri;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(byteLength),
            CLASS_FIELD(uri))

#pragma endregion
    };

    struct GltfAnimation
    {
        struct Channel
        {
            struct Target
            {
                int node;
                std::string path;

                NAU_CLASS_FIELDS(
                        CLASS_FIELD(node),
                        CLASS_FIELD(path))
            };

            int sampler;
            std::string name;
            Target target;

            NAU_CLASS_FIELDS(
                    CLASS_FIELD(sampler),
                    CLASS_FIELD(name),
                    CLASS_FIELD(target))
        };

        struct Sampler
        {
            int input;
            std::string interpolation;
            int output;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(input),
                CLASS_FIELD(interpolation),
                CLASS_FIELD(output))
        };

        std::vector<Channel> channels;
        std::string name;
        std::vector<Sampler> samplers;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(channels),
            CLASS_FIELD(name),
            CLASS_FIELD(samplers))

#pragma endregion
    };

    struct GltfSkin
    {
        std::string name;
        unsigned inverseBindMatrices;
        std::vector<unsigned> joints;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(name),
            CLASS_FIELD(inverseBindMatrices),
            CLASS_FIELD(joints))

#pragma endregion
    };

    struct GltfFile
    {
        GltfHeader asset;
        int scene = -1;
        std::vector<GltfSceneData> scenes;
        std::vector<nau::RuntimeValue::Ptr> nodes;
        std::vector<GltfMeshData> meshes;
        std::vector<GltfCameraData> cameras;
        std::vector<GltfAccessor> accessors;
        std::vector<GltfBufferView> bufferViews;
        std::vector<GltfBuffer> buffers;
        std::vector<GltfAnimation> animations;
        std::vector<GltfSkin> skins;

#pragma region Class Info

        NAU_CLASS_FIELDS(
            CLASS_FIELD(asset),
            CLASS_FIELD(scene),
            CLASS_FIELD(scenes),
            CLASS_FIELD(nodes),
            CLASS_FIELD(meshes),
            CLASS_FIELD(cameras),
            CLASS_FIELD(accessors),
            CLASS_FIELD(bufferViews),
            CLASS_FIELD(buffers),
            CLASS_FIELD(animations),
            CLASS_FIELD(skins))

#pragma endregion

        static Result<> loadFromJsonStream(const io::IStreamReader::Ptr& stream, GltfFile& gltfFile);
    };
}  // namespace nau
