// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/animation/playback/animation.h"
#include "nau/assets/asset_ref.h"
#include "nau/utils/enum/enum_reflection.h"

namespace nau::animation
{
    class AnimationController;

    NAU_DEFINE_ENUM_(PlayMode, Once, Looping, PingPong);

    struct AnimationInstanceCreationData
    {
        bool isStopped = false;
        AnimationAssetRef sourceRef;
    };

    /**
     * @brief Allows to modify animation playback and blending parameters which are individual for each animated object.
     * 
     * To modify keyframe data use animation editor. See Animation and IAnimationEditor. 
     */
    class NAU_ANIMATION_EXPORT AnimationInstance : public virtual IRefCounted
    {
        NAU_CLASS(nau::animation::AnimationInstance, rtti::RCPolicy::StrictSingleThread)

    public:

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] name         Name of the animation.
         * @param [in] animation    Animation object.
         * @param [in] creationData Optional creation data.
         */
        AnimationInstance(const std::string& name, nau::Ptr<Animation> animation, const AnimationInstanceCreationData* creationData = nullptr);

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] name         Name of the animation.
         * @param [in] assetRef     Animation asset.
         */
        AnimationInstance(const std::string& name, AnimationAssetRef&& assetRef);

        /**
         * @brief Initialization constructor.
         *
         * @param [in] name         Name of the instance.
         * @param [in] assetRef     Animation asset.
         */
        AnimationInstance(const std::string& name, const AnimationAssetRef& assetRef);

        /**
         * @brief Copy constructor.
         * 
         * @param [in] source Instance to copy.
         */
        AnimationInstance(const std::string& name, const AnimationInstance& source);

        /**
         * @brief Loads animation from the animation asset.
         */
        async::Task<> load();

        /**
         * @brief Updates animations weights and animates the target.
         * 
         * @param [in] controller   Animation controller the animation instance is assigned to.
         * @param [in] dt           Time elapsed since previous update.
         * @param [in] target       Animated object
         */
        void update(AnimationController& controller, float dt, const IAnimatable::Ptr& target);

        /**
         * @brief Resets the animation playback to beginning.
         * 
         * @param [in] controller Animation controller the animation instance is assigned to.
         */
        void restart(AnimationController& controller);

        /**
         * @brief Retrieves the number of seconds elapsed since the animation playback has been restarted.
         * 
         * @return Current playback time in seconds.
         */
        float getCurrentTime() const;
        
        /**
         * @brief Checks whether the animation is currently being played.
         * 
         * @return `true` is the animation is currently being played, `false` otherwise.
         */
        bool isPlaying() const;

        /**
         * @brief Retrieves index of the currently played frame of the animation.
         * 
         * @return Index of the currently played frame.
         */
        int getCurrentFrame() const;

        /**
         * @brief Retrieves an animation player object that the animation instance is assigned to.
         * 
         * @return A pointer to the animation player object.
         */
        IAnimationPlayer* getPlayer();

        /**
         * @brief Retrieves the playback mode that is currently set for the animation instance.
         * 
         * @return Playmode of the animation instance.
         * 
         * See @ref m_playMode.
         */
        PlayMode getPlayMode() const;

        /**
         * @brief Changes the playback mode for the animation instance.
         * 
         * @param [in] mode Playback mode to assign.
         * 
         * See @ref m_playMode.
         */
        void setPlayMode(PlayMode mode);

        /**
         * @brief Checks if the playback of the animation instance is reversed.
         * 
         * @return `true` if the playback is reversed, `false` otherwise.
         */
        bool isReversed() const;

        /**
         * @brief Sets the reverse mode of the animation instance playback.
         * 
         * @param [in] reverse Indicates whether the playback should be reversed.
         */
        void setIsReversed(bool reverse);

        /**
         * @brief Retrieves the weight of the animation instance.
         * 
         * @return Animation instance weight.
         */
        float getWeight() const;

        /**
         * @brief Changes the weight of the animation instance.
         * 
         * @param [in] weight The value to assign
         */
        void setWeight(float weight);

        /**
         * @brief Retrieves the blending method assigned to the animation instance.
         * 
         * @return Blending method of the animation instance.
         */
        AnimationBlendMethod getBlendMethod() const;

        /**
         * @brief Reassignes the blending method to the animation instance.
         * 
         * @param [in] blendMethod blending method to assign.
         */
        void setBlendMethod(AnimationBlendMethod blendMethod);

        /**
         * @brief Checks whether the instance weight is governed by the assigned controller.
         * 
         * @return `true` if the instance ignores the controller, i.e. the instance weight can be modified with setWeight. Otherwise, `false` is returned and the instance weight cannot be modified manually.
         */
        bool getIgnoresController() const;

        /**
         * @brief Order the instance to ignore the assigned controller, which allows for manual weight modifications.
         * 
         * @param [in] ignore Indicates whether the instance should ignore the assigned controller, `false` otherwise.
         */
        void debugIgnoreController(bool ignore);

        /**
         * @brief Retrieves the animation interpolation method.
         * 
         * @return Interpolation method used for this animation instance.
         */
        AnimationInterpolationMethod getInterpolationMethod() const;

        /**
         * @brief Changes the interpolation method for the animation instance.
         * 
         * @param [in] value Value to assign.
         */
        void setInterpolationMethod(AnimationInterpolationMethod value);

        /**
         * @brief Retrieves the name of the animation instance.
         * 
         * @return Instance name.
         */
        const std::string& getName() const;
        AnimationAssetRef getAssetRef() const;

    protected:
        const Animation* getAnimation() const;

    private:
        float getDurationSeconds(AnimationController& controller) const;

        void advance(AnimationController& controller, float dt);
        void updateBlendInOut(AnimationController& controller);
        void updateEvents();
        void fireEvents();

    private:
        friend class KeyFrameAnimationPlayer;

        AnimationState m_animationState;

        nau::Ptr<Animation> m_animation;
        AnimationAssetRef m_animationAsset;

        int m_frame = -1;

        /**
         * @brief Controls animation playback behavior after the last frame has been played.
         * 
         * - Once: Playback stops after the last frame has been reached.
         * - Looping: Playback restarts after the last frame has been reached.
         * - PingPong: Playback gets reversed after the last frame has been reached.
         */
        PlayMode m_playMode = PlayMode::Once;
        bool m_isLoaded = false; ///< Indicates whether the animation asset has been loaded.
        std::string m_name;
    };

}  // namespace nau::animation
