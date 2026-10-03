// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <map>
#include <memory>
#include <vector>

#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/scene_component.h"

namespace nau::animation
{
    class AnimationComponent;
    class AnimationManagerImguiController;

    class NAU_ANIMATION_EXPORT AnimationManager final : public scene::SceneComponent,  // temp solution waiting WorldComponents support
                                                        public scene::IComponentUpdate,
                                                        public scene::IComponentEvents
    {
        NAU_OBJECT(nau::animation::AnimationManager, scene::SceneComponent, scene::IComponentUpdate, scene::IComponentEvents)
        NAU_DECLARE_DYNAMIC_OBJECT

    public:
        static AnimationManager* get(scene::SceneComponent* anySceneComponent); 

        AnimationManager();
        ~AnimationManager();

        virtual void onComponentActivated() override;
        virtual void updateComponent(float dt) override;

        void registerAnimationComponent(AnimationComponent* animComponent);
        void unregisterAnimationComponent(AnimationComponent* animComponent);

    private:
        std::vector<scene::ObjectWeakRef<AnimationComponent>> m_animComponentsCache;
        std::unique_ptr<AnimationManagerImguiController> m_uiController;
    };

}  // namespace nau::animation