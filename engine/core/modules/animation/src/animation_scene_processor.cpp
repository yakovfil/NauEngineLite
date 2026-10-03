// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/animation_scene_processor.h"

#include <span>

#include "nau/animation/components/animation_component.h"
#include "nau/animation/components/skeleton_component.h"
#include "nau/assets/asset_descriptor_factory.h"
#include "nau/scene/scene_object.h"
#include "nau/service/service_provider.h"

namespace nau::animation
{
    AnimationSceneProcessor::AnimationSceneProcessor() = default;
    AnimationSceneProcessor::~AnimationSceneProcessor() = default;

    async::Task<> AnimationSceneProcessor::initService()
    {
        return async::makeResolvedTask();
    }

    async::Task<> AnimationSceneProcessor::preInitService()
    {
        return async::makeResolvedTask();
    }

    async::Task<> AnimationSceneProcessor::activateComponentsAsync([[maybe_unused]] Uid worldUid, std::span<const scene::Component*> components, [[maybe_unused]] async::Task<> barrier)
    {
        IAssetDescriptorFactory& assetManager = getServiceProvider().get<IAssetDescriptorFactory>();
        for (const scene::Component* component : components)
        {
            if (const auto* animationComponent = component->as<const AnimationComponent*>())
            {
                // nothing to do there yet
            }
            else if (const auto* const skeletonComponent = component->as<const SkeletonComponent*>())
            {
                if (auto assetView = skeletonComponent->getSkeletonAssetView(); assetView == nullptr)
                {
                    SkeletonAssetRef& skAsset = skeletonComponent->getSkeletonAsset();
                    if (!skAsset)
                    {
                        NAU_LOG("Skeleton missing");
                        continue;
                    }

                    nau::Ptr<SkeletonAssetView> skeletonAsset = co_await skAsset.getAssetViewTyped<SkeletonAssetView>();
                    if (!skeletonAsset)
                    {
                        continue;
                    }

                    const_cast<SkeletonComponent*>(skeletonComponent)->setSkeletonAssetView(skeletonAsset);
                }

                scene::SceneObject& parentObj = skeletonComponent->getParentObject();

                animation::AnimationComponent* animationComponent = parentObj.findFirstComponent<animation::AnimationComponent>();
                if (animationComponent)
                {
                    SkeletonComponent* skeletonComponentNonConst = const_cast<SkeletonComponent*>(skeletonComponent);
                    animationComponent->addAnimationTarget(skeletonComponentNonConst);
                }
            }
        }
    }

    void AnimationSceneProcessor::syncSceneState()
    {
    }

}  // namespace nau::animation
