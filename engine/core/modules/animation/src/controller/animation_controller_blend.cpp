// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/controller/animation_controller_blend.h"

#include <algorithm>
#include <string_view>

#include "nau/animation/playback/animation_instance.h"

namespace nau::animation
{
    void BlendAnimationController::update(float dt, const IAnimatable::Ptr& target)
    {
        // todo: sync playtimes here if needed (proper blend of animations with different duration)

        AnimationController::update(dt, target);

        m_animationMixer->blendAnimations(target);
        m_animationMixer->computeFinalTransforms(target);
    }

    void BlendAnimationController::addAnimation(nau::Ptr<AnimationInstance> animation)
    {
        AnimationController::addAnimation(animation);

        auto& entry = m_playbackTable.emplace_back();
        entry.id = *animation.get();
        entry.weight = animation->getWeight();
    }

    void BlendAnimationController::setWeight(TAnimDescrParam animationId, float weight)
    {
        auto it = std::find_if(m_playbackTable.begin(), m_playbackTable.end(), [&animationId](const auto& data)
        {
            return data.id == animationId;
        });

        if(it != m_playbackTable.end())
        {
            it->weight = weight;
        }
    }

    const std::string_view BlendAnimationController::getControllerTypeName() const
    {
        return "blend_skeletal";
    }

    float BlendAnimationController::getWeight(TAnimDescrParam animationId) const
    {
        auto it = std::find_if(m_playbackTable.begin(), m_playbackTable.end(), [&animationId](const auto& data)
        {
            return data.id == animationId;
        });

        return it != m_playbackTable.end() ? it->weight : .0f;
    }

}  // namespace nau::animation
