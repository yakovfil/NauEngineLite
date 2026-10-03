// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/controller/animation_controller_direct.h"

#include <algorithm>
#include <string_view>

#include "nau/animation/playback/animation_instance.h"

namespace nau::animation
{
    void DirectAnimationController::update(float dt, const IAnimatable::Ptr& target)
    {
        updateWeights();

        AnimationController::update(dt, target);
    }

    void DirectAnimationController::addAnimation(nau::Ptr<AnimationInstance> animation)
    {
        AnimationController::addAnimation(animation);

        auto& entry = m_playbackTable.emplace_back();
        entry.id = *animation.get();

        entry.desiredWeight = animation->getWeight();
    }

    void DirectAnimationController::setWeight(TAnimDescrParam animationId, float weight)
    {
        auto it = std::find_if(m_playbackTable.begin(), m_playbackTable.end(), [&animationId](const auto& data)
        {
            return data.id == animationId;
        });

        if(it != m_playbackTable.end())
        {
            it->desiredWeight = weight;
        }
    }

    const std::string_view DirectAnimationController::getControllerTypeName() const
    {
        return "direct";
    }

    float DirectAnimationController::getWeight(TAnimDescrParam animationId) const
    {
        auto it = std::find_if(m_playbackTable.begin(), m_playbackTable.end(), [&animationId](const auto& data)
        {
            return data.id == animationId;
        });

        return it != m_playbackTable.end() ? it->weight : .0f;
    }

    void DirectAnimationController::updateWeights()
    {
        float fullWeight = .0f;

        for(const auto& animData : m_playbackTable)
        {
            fullWeight += animData.desiredWeight;
        }

        for(auto& animData : m_playbackTable)
        {
            animData.weight = fullWeight > NEGLIGIBLE_WEIGHT ? animData.desiredWeight / fullWeight : .0f;
        }
    }

}  // namespace nau::animation
