// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <vector>

#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/scene_component.h"

namespace nau::animation
{
    class AnimationComponent;
    class AnimationManager;

    class AnimationManagerImguiController final 
    {
    public:
        AnimationManagerImguiController(AnimationManager& owner);

        void drawGui(const std::vector<scene::ObjectWeakRef<AnimationComponent>>& animComponents);

    private:
        int m_selectedAnimComponentIndex = -1;
        int m_selectedTrackIndex = -1;
        int m_selectedSkeletonComponentIndex = -1;
        std::string m_name;
    };

}  // namespace nau::animation