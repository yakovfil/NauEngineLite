// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./gltf_scene_asset.h"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/assets/asset_ref.h"
#include "nau/scene/components/camera_component.h"
#include "nau/scene/components/skinned_mesh_component.h"
#include "nau/scene/components/static_mesh_component.h"
#include "nau/serialization/runtime_value.h"
#include "nau/service/service_provider.h"

namespace nau
{
    namespace details
    {
        static std::map<int, std::vector<GltfAnimationParseInfo>> parseKeyframeAnimationsGltf(const GltfFile& gltfFile, const std::set<unsigned>& skeletonJointNodes)
        {
            std::map<int, std::vector<details::GltfAnimationParseInfo>> animParseInfos;

            for (size_t animIdx = 0; animIdx < gltfFile.animations.size(); ++animIdx)
            {
                const GltfAnimation& animation = gltfFile.animations[animIdx];

                bool isSkeletalAnim = false;

                for (size_t channelIdx = 0; channelIdx < animation.channels.size(); ++channelIdx)
                {
                    const GltfAnimation::Channel& channel = animation.channels[channelIdx];
                    const int targetIndex = channel.target.node;
                    const bool isJoint = std::find(skeletonJointNodes.begin(), skeletonJointNodes.end(), targetIndex) != skeletonJointNodes.end();

                    isSkeletalAnim |= isJoint;
                    if (isSkeletalAnim)
                    {
                        break;
                    }
                }

                if (isSkeletalAnim)
                {
                    // skeletal animation tracks currently can't be mixed with regular animation tracks in same Animation
                    continue;
                }

                for (size_t channelIdx = 0; channelIdx < animation.channels.size(); ++channelIdx)
                {
                    const GltfAnimation::Channel& channel = animation.channels[channelIdx];

                    int targetIndex = channel.target.node;

                    auto& animParseInfo = animParseInfos[targetIndex].emplace_back();

                    animParseInfo.animName = animation.name;
                    animParseInfo.targetPath = channel.target.path;

                    animParseInfo.animIndex = animIdx;
                    animParseInfo.channelIndex = channelIdx;
                }
            }

            return animParseInfos;
        }

        static SkinnedMeshParseInfo parseSkinnedMeshSkeletonAndAnimationsGltfNode(RuntimeReadonlyDictionary& gltfNode)
        {
            if (!gltfNode.containsKey("extras"))
            {
                return {};
            }

            SkinnedMeshParseInfo result;

            auto extras = gltfNode["extras"];
            auto& extrasNode = extras->as<RuntimeReadonlyDictionary&>();

            if (extrasNode.containsKey("ozz_skeleton_path"))
            {
                const auto& skValue = extrasNode.getValue("ozz_skeleton_path");
                result.skeletonPath = *runtimeValueCast<std::string>(skValue);
            }

            const bool containsAnimationsBlock = extrasNode.containsKey("ozz_animations");
            if (!containsAnimationsBlock)
            {
                return result;
            }

            auto animations = extrasNode["ozz_animations"];
            auto& animationsNode = animations->as<RuntimeReadonlyCollection&>();
            auto collectionSize = animationsNode.getSize();
            for (size_t i = 0; i < collectionSize; ++i)
            {
                auto anim = animationsNode.getAt(i);
                auto& animNode = anim->as<RuntimeReadonlyDictionary&>();
                const bool containsPath = animNode.containsKey("path");
                const bool containsBlendMethod = animNode.containsKey("blend_method");
                const bool containsWeight = animNode.containsKey("weight");
                if (containsPath && containsBlendMethod && containsWeight)
                {
                    auto& newAnim = result.animations.emplace_back();
                    newAnim.ozzPath = *runtimeValueCast<std::string>(animNode["path"]);
                    newAnim.weight = *runtimeValueCast<float>(animNode["weight"]);
                    newAnim.blendMethod = *runtimeValueCast<std::string>(animNode["blend_method"]);
                }
                else
                {
                    NAU_ASSERT(false);
                }
            }

            return result;
        }
    }  // namespace details

    RuntimeValue::Ptr makeAssetPath(IAssetContainer& container, IAssetDescriptorFactory& assetFactory, std::string_view assetInnerPath)
    {
        IAssetDescriptor::Ptr asset = assetFactory.createAssetDescriptor(container, strings::toStringView(assetInnerPath));
        return makeValueCopy(asset->getAssetPath());
    }

    ComponentAsset& GltfSceneAsset::GltfSceneObjectAsset::addComponent(const rtti::TypeInfo& type)
    {
        auto& component = additionalComponents.emplace_back();
        component.uid = Uid::generate();
        component.setComponentType(type);

        return (component);
    }

    ComponentAsset& GltfSceneAsset::GltfSceneObjectAsset::getExistingComponentOrAdd(const rtti::TypeInfo& type)
    {
        auto existingComponent = std::find_if(additionalComponents.begin(), additionalComponents.end(), [&type](const ComponentAsset& comp)
        {
            return comp.getComponentType() == type;
        });

        if (existingComponent != additionalComponents.end())
        {
            return *existingComponent;
        }

        return addComponent(type);
    }

    GltfSceneAsset::GltfSceneObjectAsset* GltfSceneAsset::GltfSceneObjectAsset::findNodeByIndex(unsigned index)
    {
        if (nodeIndex == index)
        {
            return this;
        }

        auto childIter = std::find_if(children.begin(), children.end(), [index](const GltfSceneObjectAsset& child)
        {
            return child.nodeIndex == index;
        });

        if (childIter != children.end())
        {
            return &(*childIter);
        }

        for (GltfSceneObjectAsset& child : children)
        {
            if (GltfSceneObjectAsset* object = child.findNodeByIndex(index); object)
            {
                return object;
            }
        }

        return nullptr;
    }

    GltfSceneAsset::GltfSceneAsset(IAssetContainer& container, const GltfFile& gltfFile, unsigned sceneIndex)
    {
        NAU_FATAL(sceneIndex < gltfFile.scenes.size());
        const auto& gltfScene = gltfFile.scenes[sceneIndex];

        m_root.rootComponent.uid = NullUid;
        m_root.additionalComponentCount = 0;
        m_root.childCount = gltfScene.nodes.size();
        m_root.children.reserve(gltfScene.nodes.size());

        std::set<unsigned> skeletonJointNodes = {};
        for (const auto& skin : gltfFile.skins)
        {
            for (const auto jointNodeIndex : skin.joints)
            {
                skeletonJointNodes.insert(jointNodeIndex);
            }
        }

        for (const unsigned nodeIndex : gltfScene.nodes)
        {
            // do not add skeleton joints as separate objects to scene, use SkeletonSocketComponent
            if (skeletonJointNodes.count(nodeIndex) == 0)
            {
                m_root.children.emplace_back(makeSceneObjectAsset(container, gltfFile, skeletonJointNodes, nodeIndex));
            }
        }
        const auto animParseInfos = details::parseKeyframeAnimationsGltf(gltfFile, skeletonJointNodes);
        makeAnimationComponentAssets(container, animParseInfos);
    }

    void GltfSceneAsset::makeAnimationComponentAssets(IAssetContainer& container, const std::map<int, std::vector<details::GltfAnimationParseInfo>>& animationParseInfos)
    {
        IAssetDescriptorFactory& assetFactory = getServiceProvider().get<IAssetDescriptorFactory>();

        for (const auto& pair : animationParseInfos)
        {
            const int targetNodeIndex = pair.first;
            const std::vector<details::GltfAnimationParseInfo>& animsTracks = pair.second;

            GltfSceneObjectAsset* const targetObject = m_root.findNodeByIndex(targetNodeIndex);
            if (!targetObject)
            {
                NAU_LOG_WARNING("Invalid gltf target node index: ({})", targetNodeIndex);
                continue;
            }

            ComponentAsset& animationComponentAsset = targetObject->addComponent(rtti::getTypeInfo<animation::AnimationComponent>());

            std::vector<RuntimeValue::Ptr> animTracks;

            for (const details::GltfAnimationParseInfo& trackInfo : animsTracks)
            {
                auto formattedGltfAnimName = fmt::format("animation/{}/{}", trackInfo.animIndex, trackInfo.channelIndex);
                IAssetDescriptor::Ptr animAssetDescriptor = assetFactory.createAssetDescriptor(container, strings::toStringView(formattedGltfAnimName));
                AssetPath assetPath = animAssetDescriptor->getAssetPath();

                nau::animation::AnimTrackCreationInfo ti{
                    trackInfo.animName,
                    "PingPong",  // default PlayMode for gltf keyframe anims import
                    1.0f,
                    trackInfo.targetPath,
                    "",
                    assetPath};

                animTracks.emplace_back(makeValueCopy(ti));
            }

            RuntimeStringValue::Ptr controllerTypeStr = makeValueCopy(std::string_view("direct"));
            RuntimeCollection::Ptr tracksCollection = makeValueCopy(std::move(animTracks));

            std::map<std::string, RuntimeValue::Ptr> animationComponentProps = {
                {"animControllerType", controllerTypeStr},
                {"tracksCreationInfo",  tracksCollection}
            };

            animationComponentAsset.properties = makeValueCopy(std::move(animationComponentProps));
        }
    }

    void GltfSceneAsset::makeSkeletalAnimationComponentAsset(IAssetContainer& container, GltfSceneObjectAsset& object, const details::SkinnedMeshParseInfo& skinnedMeshParseInfo)
    {
        IAssetDescriptorFactory& assetFactory = getServiceProvider().get<IAssetDescriptorFactory>();

        ComponentAsset& animationComponentAsset = object.addComponent(rtti::getTypeInfo<animation::AnimationComponent>());

        std::vector<RuntimeValue::Ptr> animTracks;
        for (const auto& anim : skinnedMeshParseInfo.animations)
        {
            auto formattedOzzAnimName = fmt::format("skeletal_animation_ozz/{}", anim.ozzPath);
            IAssetDescriptor::Ptr animAssetDescriptor = assetFactory.createAssetDescriptor(container, strings::toStringView(formattedOzzAnimName));
            AssetPath assetPath = animAssetDescriptor->getAssetPath();

            nau::animation::AnimTrackCreationInfo ti{
                anim.ozzPath,
                "Looping",  // default PlayMode for ozz skeletal anims import
                anim.weight,
                "",
                anim.blendMethod,
                assetPath};

            animTracks.emplace_back(makeValueCopy(ti));
        }

        RuntimeStringValue::Ptr controllerTypeStr = makeValueCopy(std::string_view("blend_skeletal"));
        RuntimeCollection::Ptr tracksCollection = makeValueCopy(std::move(animTracks));

        std::map<std::string, RuntimeValue::Ptr> animationComponentProps = {
            {"animControllerType", controllerTypeStr},
            {"tracksCreationInfo",  tracksCollection}
        };

        animationComponentAsset.properties = makeValueCopy(std::move(animationComponentProps));
    }

    GltfSceneAsset::GltfSceneObjectAsset GltfSceneAsset::makeSceneObjectAsset(IAssetContainer& container, const GltfFile& gltfFile, const std::set<unsigned>& skeletonJointNodes, unsigned index)
    {
        NAU_FATAL(index < gltfFile.nodes.size());
        RuntimeReadonlyDictionary& gltfNode = gltfFile.nodes[index]->as<RuntimeReadonlyDictionary&>();
        GltfNodeBase node = *runtimeValueCast<GltfNodeBase>(gltfNode.as<RuntimeValue*>());

        GltfSceneObjectAsset object;
        object.nodeIndex = index;
        object.name = node.name;
        object.uid = Uid::generate();
        object.rootComponent = makeComponentAsset(container, gltfFile, object, gltfNode, node);
        object.childCount = node.children.size();
        object.children.reserve(object.childCount);

        for (unsigned childIndex : node.children)
        {
            // we do not add skeleton joints as separate objects to scene, use SkeletonSocketComponent
            if (skeletonJointNodes.count(childIndex) != 0)
            {
                continue;
            }
            object.children.emplace_back(makeSceneObjectAsset(container, gltfFile, skeletonJointNodes, childIndex));
        }

        return object;
    }

    ComponentAsset GltfSceneAsset::makeComponentAsset(IAssetContainer& container, const GltfFile& gltfFile, GltfSceneObjectAsset& object, RuntimeReadonlyDictionary& gltfNode, const GltfNodeBase& node)
    {
        IAssetDescriptorFactory& assetFactory = getServiceProvider().get<IAssetDescriptorFactory>();

        ComponentAsset component = {
            .componentTypeId = 0,
            .uid = Uid::generate()};

        std::map<std::string, RuntimeValue::Ptr> properties;

        if (gltfNode.containsKey("mesh"))
        {
            const auto meshIndex = *runtimeValueCast<unsigned>(gltfNode["mesh"]);

            properties["geometry"] = makeAssetPath(container, assetFactory, fmt::format("mesh/{}", meshIndex));

            if (const bool isSkinned = gltfNode.containsKey("skin"); !isSkinned)
            {
                component.setComponentType<scene::StaticMeshComponent>();
            }
            else  // isSkinned
            {
                component.setComponentType<SkinnedMeshComponent>();
                const unsigned skinIdx = *runtimeValueCast<unsigned>(gltfNode["skin"]);

                details::SkinnedMeshParseInfo skinningParseInfo = details::parseSkinnedMeshSkeletonAndAnimationsGltfNode(gltfNode);

                if (!skinningParseInfo.skeletonPath.empty())
                {
                    ComponentAsset& skeletonComponent = object.addComponent(rtti::getTypeInfo<SkeletonComponent>());
                    std::map<std::string, RuntimeValue::Ptr> skeletonProps = {
                        // load skeleton from gltf + ozz
                        {"skeletonAsset", makeAssetPath(container, assetFactory, fmt::format("skin/{}/{}", skinIdx, skinningParseInfo.skeletonPath))}
                    };

                    skeletonComponent.properties = makeValueCopy(std::move(skeletonProps));  // see setSkeletonAsset

                    if (!skinningParseInfo.animations.empty())
                    {
                        makeSkeletalAnimationComponentAsset(container, object, skinningParseInfo);
                    }
                }
                else
                {
                    NAU_ASSERT(false, "Skeleton Path is missing (.ozz)");
                }
            }
        }
        else if (gltfNode.containsKey("camera"))
        {
            const auto cameraIndex = *runtimeValueCast<unsigned>(gltfNode["camera"]);

            NAU_FATAL(cameraIndex < gltfFile.cameras.size());
            const auto& cameraData = gltfFile.cameras[cameraIndex];

            component.setComponentType<scene::CameraComponent>();

            if (cameraData.perspective)
            {
                properties = {
                    {  "FieldOfView",  makeValueCopy(cameraData.perspective->yFov)},
                    {"ClipNearPlane", makeValueCopy(cameraData.perspective->zNear)},
                    { "ClipFarPlane",  makeValueCopy(cameraData.perspective->zFar)}
                };
            }
        }

        component.transform = math::Transform{
            math::quat{node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]},
            math::vec3{node.translation[0], node.translation[1], node.translation[2]},
            math::vec3{node.scale[0], node.scale[1], node.scale[2]}
        };

        if (!properties.empty())
        {
            component.properties = makeValueCopy(std::move(properties));
        }

        return component;
    }

    SceneAssetInfo GltfSceneAsset::getSceneInfo() const
    {
        return SceneAssetInfo{
            .assetKind = SceneAssetKind::Scene,
            .name = "GLTF Scene"};
    }

    std::optional<Vector<ReferenceField>> GltfSceneAsset::getReferencesInfo() const
    {
        return std::nullopt;
    }

    void GltfSceneAsset::visitScene(ISceneAssetVisitor& visitor) const
    {
        visitObjectInternal(visitor, m_root);
    }

    void GltfSceneAsset::visitObjectInternal(ISceneAssetVisitor& visitor, const GltfSceneObjectAsset& objectAsset) const
    {
        for (const GltfSceneObjectAsset& childObject : objectAsset.children)
        {
            visitor.visitSceneObject(objectAsset.uid, childObject);
        }

        for (const ComponentAsset& component : objectAsset.additionalComponents)
        {
            visitor.visitSceneComponent(objectAsset.uid, component);
        }

        for (const GltfSceneObjectAsset& child : objectAsset.children)
        {
            visitObjectInternal(visitor, child);
        }
    }
}  // namespace nau