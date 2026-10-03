// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string_view>
#include <vector>

#include "nau/animation/controller/animation_controller.h"

namespace nau::animation
{
    class NAU_ANIMATION_EXPORT DirectAnimationController : public AnimationController
    {
        NAU_CLASS(nau::animation::DirectAnimationController, rtti::RCPolicy::StrictSingleThread, AnimationController)
        
    public:
        virtual void update(float dt, const IAnimatable::Ptr& target) override;

        virtual void addAnimation(nau::Ptr<AnimationInstance> animation) override;
        
        virtual float getWeight(TAnimDescrParam animationId) const override;
        void setWeight(TAnimDescrParam animationId, float weight);

        virtual const std::string_view getControllerTypeName() const override;

    protected:
        void updateWeights();

    private:
        struct AnimationPlaybackData
        {
            TAnimDescr id;
            float desiredWeight = .0f;
            float weight = .0f;
        };

        std::vector<AnimationPlaybackData> m_playbackTable;
    };
} // namespace nau::animation
