// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "./gltf_file.h"
#include "nau/assets/asset_container.h"
#include "nau/assets/scene_asset.h"
#include "nau/memory/standard_aliases.h"

namespace nau
{
    namespace details
    {
        struct GltfAnimationParseInfo
        {
            std::string animName;
            std::string targetPath;

            size_t animIndex;
            size_t channelIndex;
        };

        struct SkeletalAnimParseInfo
        {
            std::string ozzPath;
            std::string blendMethod;
            float weight;
        };
        struct SkinnedMeshParseInfo
        {
            std::string skeletonPath = "";
            std::vector<SkeletalAnimParseInfo> animations;
        };
    }

    class GltfSceneAsset final : public SceneAsset
    {
        NAU_CLASS_(nau::GltfSceneAsset, SceneAsset)

    public:
        GltfSceneAsset(IAssetContainer& container, const GltfFile& gltfFile, unsigned sceneIndex);

    private:
        struct GltfSceneObjectAsset : SceneObjectAsset
        {
            static constexpr unsigned NotIndex = std::numeric_limits<unsigned>::max();
            unsigned nodeIndex = NotIndex;
            std::vector<GltfSceneObjectAsset> children;
            std::vector<ComponentAsset> additionalComponents;

            ComponentAsset& addComponent(const rtti::TypeInfo& type);
            ComponentAsset& getExistingComponentOrAdd(const rtti::TypeInfo& type);
            GltfSceneObjectAsset* findNodeByIndex(unsigned index);
        };

        GltfSceneObjectAsset makeSceneObjectAsset(IAssetContainer& container, const GltfFile& gltfFile, const std::set<unsigned>& skeletonJointNodes, unsigned index);

        ComponentAsset makeComponentAsset(IAssetContainer& container, const GltfFile& gltfFile, GltfSceneObjectAsset& object, RuntimeReadonlyDictionary& nodeData, const GltfNodeBase& node);

        SceneAssetInfo getSceneInfo() const override;

        std::optional<Vector<ReferenceField>> getReferencesInfo() const override;

        void visitScene(ISceneAssetVisitor& visitor) const override;

        void visitObjectInternal(ISceneAssetVisitor& visitor, const GltfSceneObjectAsset& objectAsset) const;

        void makeAnimationComponentAssets(IAssetContainer& container, const std::map<int, std::vector<details::GltfAnimationParseInfo>>& animationParseInfos);
        void makeSkeletalAnimationComponentAsset(IAssetContainer& container, GltfSceneObjectAsset& object, const details::SkinnedMeshParseInfo& skinnedMeshParseInfo);

        GltfSceneObjectAsset m_root;
    };
}  // namespace nau
