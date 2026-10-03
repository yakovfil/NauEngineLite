// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/animation_manager.h"

#include <algorithm>
#include <memory>

#include "instruments/animation_manager_ui_controller.h"
#include "nau/animation/components/animation_component.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_object.h"

namespace nau::animation
{

    NAU_IMPLEMENT_DYNAMIC_OBJECT(AnimationManager)

    AnimationManager* AnimationManager::get(SceneComponent* anySceneComponent)
    {
        if (anySceneComponent)
        {
            auto& sceneObject = anySceneComponent->getParentObject();

            if (auto* scene = sceneObject.getScene())
            {
                return scene->getRoot().findFirstComponent<AnimationManager>(true);
            }
        }
        return nullptr;
    }

    AnimationManager::AnimationManager() = default;

    AnimationManager::~AnimationManager() = default;

    void AnimationManager::onComponentActivated()
    {
        m_uiController = std::make_unique<AnimationManagerImguiController>(*this);
    }

    void AnimationManager::updateComponent(float dt)
    {
        if (auto* uiController = m_uiController.get())
        {
            uiController->drawGui(m_animComponentsCache);
        }
    }

    void AnimationManager::registerAnimationComponent(AnimationComponent* animComponent)
    {
        m_animComponentsCache.push_back(*animComponent);
    }

    void AnimationManager::unregisterAnimationComponent(AnimationComponent* animComponent)
    {
        auto iter = std::remove_if(m_animComponentsCache.begin(), m_animComponentsCache.end(), [=](const auto& it)
        {
            return &*it == animComponent;
        });

        m_animComponentsCache.erase(iter);
    }

}  // namespace nau::animation