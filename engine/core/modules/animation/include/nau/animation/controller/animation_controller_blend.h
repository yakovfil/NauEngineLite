// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string_view>
#include <vector>

#include "nau/animation/controller/animation_controller.h"
#include "nau/animation/playback/animation_mixer.h"

namespace nau::animation
{
    class NAU_ANIMATION_EXPORT BlendAnimationController : public AnimationController
    {
        NAU_CLASS(nau::animation::BlendAnimationController, rtti::RCPolicy::StrictSingleThread, AnimationController)
        
    public:

        /**
         * @brief Default constructor (deleted).
         */
        BlendAnimationController() = delete;

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] aMixer Mixer object that is used to blend the managed animations.
         */
        BlendAnimationController(nau::Ptr<AnimationMixer> aMixer) : m_animationMixer(aMixer) {};

        /**
         * @brief Advances each managed animation instance, blends them and animates the target.         
         * 
         * @param [in] dt       Time in seconds elapsed since the last update.
         * @param [in] target   Target object to apply animations to.
         */
        virtual void update(float dt, const IAnimatable::Ptr& target) override;

        /**
         * @brief Binds the animation to the controller.
         *
         * @param [in] animation Animation to manage.
         */
        virtual void addAnimation(nau::Ptr<AnimationInstance> animation) override;
        
        /**
         * @brief Retrieves the weight of a managed animation.
         *
         * @param [in] animationId A handle to the animation instance to retrieve the weight of.
         * @return Animation instance weight.
         */
        virtual float getWeight(TAnimDescrParam animationId) const override;

        /**
         * @brief Sets the weight of the managed animation.
         *
         * @param [in] animationId A handle to the animation instance to set the weight of.
         * @param [in] weight Value to assign.
         */
        void setWeight(TAnimDescrParam animationId, float weight);

        virtual const std::string_view getControllerTypeName() const override;

    private:

        struct AnimationPlaybackData
        {
            TAnimDescr id;
            float weight = .0f;
        };

        /**
         * @brief Stores weights of managed animations.
         */
        std::vector<AnimationPlaybackData> m_playbackTable;

        /**
         * @brief Mixer object that is responsible for blending animation instances.
         */
        nau::Ptr<AnimationMixer> m_animationMixer;
    };
} // namespace nau::animation
