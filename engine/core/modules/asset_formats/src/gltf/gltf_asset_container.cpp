// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "gltf/gltf_asset_container.h"

#include <string>
#include <string_view>
#include <vector>

#include "animation/gltf_animation_accessor.h"
#include "animation/nanim_animation_accessor.h"
#include "gltf/gltf_mesh_accessor.h"
#include "gltf/gltf_scene_asset.h"
#include "gltf/gltf_skin_accessor.h"
#include "nau/animation/assets/animation_asset.h"
#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/animation/controller/animation_controller_blend.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/animation/playback/animation_skeleton.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/scene/components/camera_component.h"
#include "nau/scene/components/skinned_mesh_component.h"
#include "nau/scene/components/static_mesh_component.h"
#include "nau/scene/scene_factory.h"
#include "nau/scene/scene_object.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_conv.h"

namespace nau
{
    GltfAssetContainer::GltfAssetContainer(GltfFile gltfFile, const io::FsPath& filePath) :
        m_gltfFile(std::move(gltfFile))
        , m_gltfFilePath(filePath)
    {
        const auto dirPath = filePath.getParentPath();
        NAU_ASSERT(m_gltfFile.buffers.empty() || !dirPath.isEmpty());

        auto& fileSystem = getServiceProvider().get<io::IFileSystem>();

        const std::string_view binFileName = filePath.getStem();

        m_bufferFiles.reserve(m_gltfFile.buffers.size());
        for(const auto& buffer : m_gltfFile.buffers)
        {
            std::string path = buffer.uri.empty() ? "" : std::string(binFileName.cbegin(), binFileName.cend()) + ".bin";

            io::FsPath bufferPath = dirPath / path;
            auto file = fileSystem.openFile(bufferPath, io::AccessMode::Read, io::OpenFileMode::OpenExisting);
            m_bufferFiles.emplace_back(std::move(file));
        }
    }

    nau::Ptr<> GltfAssetContainer::getAsset(std::string_view path)
    {
        if(path.empty())
        {
            NAU_ASSERT(!m_gltfFile.scenes.empty());
            if (m_gltfFile.scenes.empty())
            {
                return nullptr;
            }

            const auto sceneIndex = (m_gltfFile.scene >=0 && m_gltfFile.scene < m_gltfFile.scenes.size()) ? static_cast<size_t>(m_gltfFile.scene) : 0;
            return rtti::createInstance<GltfSceneAsset>(*this, m_gltfFile, sceneIndex);
        }

        if (path == "kfanimation")
        {
            auto& extras = m_gltfFile.asset.extras->as<RuntimeReadonlyDictionary&>();

            if (auto animationsUidValue = extras.getValue("animationsUid"))
            {
                const auto animationsUid = *runtimeValueCast<std::string>(animationsUidValue);
                io::FsPath animationsFilePath = m_gltfFilePath.getParentPath() / animationsUid;

                return rtti::createInstance<NanimAnimationAssetAccessor, IAssetAccessor>(animationsFilePath, 0, 0);
            }

            return rtti::createInstance<NanimAnimationAssetAccessor, IAssetAccessor>(m_gltfFilePath, 0, 0);
        }

        const auto [kind, name] = strings::cut(path, '/');
        if(kind.empty() || name.empty())
        {
            return nullptr;
        }

        if(kind == "scene")
        {
            NAU_ASSERT(!m_gltfFile.scenes.empty());

            const auto sceneIndex = !name.empty() ? strings::lexicalCast<size_t>(name) : static_cast<size_t>(m_gltfFile.scene);
            NAU_ASSERT(sceneIndex < m_gltfFile.scenes.size());
            if ( m_gltfFile.scenes.size() <= sceneIndex)
            {
                return nullptr;
            }

            return rtti::createInstance<GltfSceneAsset>(*this, m_gltfFile, sceneIndex);
        }
        else if(kind == "mesh")
        {
            const auto meshIndex = !name.empty() ? strings::lexicalCast<size_t>(name) : 0;
            NAU_ASSERT(meshIndex < m_gltfFile.meshes.size());
            if(meshIndex >= m_gltfFile.meshes.size())
            {
                NAU_ASSERT(false);
                return nullptr;
            }

            return rtti::createInstance<GltfMeshAssetAccessor, IAssetAccessor>(m_gltfFile, meshIndex, m_bufferFiles);
        }
        else if(kind == "camera")
        {
        }
        else if (kind == "skin")
        {
            const auto [skinIndexStr, skeletonPath] = strings::cut(name, '/');
            const auto skinIndex = !skinIndexStr.empty() ? strings::lexicalCast<size_t>(skinIndexStr) : 0;
            NAU_ASSERT(skinIndex < m_gltfFile.skins.size());
            if(skinIndex >= m_gltfFile.skins.size())
            {
                NAU_ASSERT(false);
                return nullptr;
            }

            return rtti::createInstance<GltfSkinAssetAccessor, IAssetAccessor>(m_gltfFile, skinIndex, skeletonPath, m_bufferFiles);
        }
        else if (kind == "animation")
        {
            const auto [animation, channel] = strings::cut(name, '/');

            if(!animation.empty() && !channel.empty())
            {
                auto animationIndex = strings::lexicalCast<size_t>(animation);
                auto channelIndex = strings::lexicalCast<size_t>(channel);

                NAU_ASSERT(animationIndex < m_gltfFile.animations.size());
                if(animationIndex < m_gltfFile.animations.size())
                {
                    NAU_ASSERT(channelIndex < m_gltfFile.animations[animationIndex].channels.size());

                    if(channelIndex < m_gltfFile.animations[animationIndex].channels.size())
                    {
                        return rtti::createInstance<GltfAnimationAssetAccessor, IAssetAccessor>(m_gltfFile, animationIndex, channelIndex, m_bufferFiles);
                    }
                }
            }
        }
        else if (kind == "skeletal_animation_ozz")
        {
            auto assetView = animation::data::AnimationAssetView::createFromOzzPath(name);

            return assetView;
        }

        return nullptr;
    }

    std::vector<std::string> GltfAssetContainer::getContent() const
    {
        return {};
    }

    std::vector<std::string_view> GltfAssetContainerLoader::getSupportedAssetKind() const
    {
        return {"Model/*", "gltf"};
    }

    async::Task<IAssetContainer::Ptr> GltfAssetContainerLoader::loadFromStream(io::IStreamReader::Ptr stream, AssetContentInfo info)
    {
        NAU_ASSERT(stream);
        ASYNC_SWITCH_EXECUTOR(async::Executor::getDefault())

        GltfFile gltfFile;

        co_await GltfFile::loadFromJsonStream(stream, gltfFile);
        co_return rtti::createInstance<GltfAssetContainer>(std::move(gltfFile), info.path);
    }

    RuntimeReadonlyDictionary::Ptr GltfAssetContainerLoader::getDefaultImportSettings() const
    {
        return nullptr;
    }
}  // namespace nau
