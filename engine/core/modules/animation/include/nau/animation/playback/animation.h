// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <array>
#include <span>
#include <string>
#include <vector>

#include "nau/animation/data/frame.h"
#include "nau/animation/data/keyframe.h"
#include "nau/animation/interfaces/animatable.h"
#include "nau/animation/interfaces/animation_player.h"
#include "nau/dataBlock/dag_dataBlock.h"
#include "nau/scene/nau_object.h"

namespace nau::scene
{
    class SceneObject;
}

namespace nau::animation
{
    class AnimationInstance;

    /**
     * @brief Enumerates methods to interpolate values between two keyframes.
     */
    enum class AnimationInterpolationMethod
    {
        Step,   ///<Resulted value is a value from the earlier of two keyframes.
        Linear  ///<Resulted value is a linear interpolation between values from two keyframes.
    };

    /**
     * @brief Enumerates methods to blend between multiple animations.
     */
    enum class AnimationBlendMethod
    {
        Mix,        ///<Resulted value is a linear interpolation between blended values.
        Additive    ///<Resulted value is a weighted sum of blended values.
    };

    /**
     * @brief Enumerates frame event control flags.
     */
    enum class FrameEventControl : uint8_t
    {
        IsActive = NauFlag(1),  ///< Indicates whether the event will fire the associated action when triggered.
        IsPinned = NauFlag(2)   ///< Events become inactive once they are triggered, unless they are pinned. You have to deactivate pinned events manually. Pinned events still get deactivated once the playback reaches the last frame.
    };

    NAU_DEFINE_TYPED_FLAG(FrameEventControl)

    /**
     * @brief Encapsualates frame event data.
     */
    struct FrameEventInfo
    {
        std::string id;
        FrameEventControlFlag flags = { };
    };

    inline constexpr int MAX_EVENTS_PER_FRAME = 16;

    /**
     * @brief Describes animation playback state relevant for the current frame.
     */
    struct AnimationState
    {
        /**
         * @brief Current playback time.
         */
        float time;

        /**
         * @brief Current animation playback speed.
         */
        float playbackSpeed = 1.f;

        /**
         * @brief Index of the frame this animation will be reset to at the next update. If set to negative value, no reset will occur.
         */
        int forcedFrame = -1; 

        /**
         * @brief Index of the last processed keyframe.
         * 
         * When we want to pinpoint the two closest keyframes a frame is located between, the API will begin search
         * from this frame as it is expected that keyframes are animated through one by one.
         */
        int baseKeyFrameIndex = 0;

        /**
         * @brief A pointer to the object to animate.
         */
        IAnimatable::Ptr target;

        /**
         * @brief A pointer to the player controlling the animation playback.
         */
        IAnimationPlayer::Ptr player;

        /**
         * @brief Identifier of the associated animation instance.
         * 
         * @todo Change identifier string representation to UID.
         */
        nau::string animInstanceName;
        
        /**
         * @brief Animation weight that is regarded as its relative contribution to the resulted value of the animated parameter when accumulating (blending) influences from multiple animations.
         * 
         * The less the weight of the animation is, the less impact on the resulted value of the animated parameter the animation will make.
         */
        float weight = 1.f;

        /**
         * @brief Animation weight that is changed during the blending-in or blending-out process.
         * 
         * The less the weight of the animation is, the less impact on the resulted value of the animated parameter the animation will make.
         */
        float blendInOutWeight = 1.f;
      
        /**
         * @brief Duration of blending-in process for the animation.
         * 
         * Blending-in is a process when an animation weight gradually increases from 0 to its maximum.
         * See @ref blendInOutWeight.
         */
        float blendInTime = 0.f;

        /**
         * @brief Duration of blending-out process for the animation.
         * 
         * Blending-out is a process when an animation weight gradually decreases from its maximum to zero.
         * See @ref blendInOutWeight.
         */
        float blendOutTime = 0.f;

        AnimationInterpolationMethod interpolationMethod = AnimationInterpolationMethod::Linear;
        AnimationBlendMethod blendMethod = AnimationBlendMethod::Mix;

        bool isReversed = false;

        bool ignoreController = false;
        bool isPaused = false;
        bool isStopped = false;

        /**
         * @brief A collection of frame events that has been triggered at this frame.
         */
        std::array<FrameEventInfo, MAX_EVENTS_PER_FRAME> events;
      

        /**
         * @brief Retrieves the animation total weight, taking blending-in/blending-out into account.
         * 
         * @return Animation total weight.
         */
        float getFullWeight() const;

        /**
         * @brief Adds the event to a frame event slot. It is marked as active.
         * 
         * @param [in] id   Event identifier, i.e. a string message that will be broadcasted.
         * @param [in] pin  Indicates whether the event should be pinned. See FrameEventControl::IsPinned
         * @return          `true` on success, `false` otherwise.
         * 
         * @note    A limitation on maximal number of active events per frame is determined by MAX_EVENTS_PER_FRAME. 
                    In case there are no inactive slots, the function will fail.

         * Active events of the animation state are processed by AnimationInstance. 
         */
        bool addEvent(const std::string& id, bool pin);

        /**
         * @brief Removes the event from a frame event slot.
         * 
         * @param [in] id   Identfier of the event to remove. 
         * @return          `true` on success, `false` otherwise (i.e. the identifier was not found).
         * 
         * Use this method to manually deactivate pinned events. Unpinned events perish each update and removed automatically.
         */
        bool removeEvent(const std::string& id);

        /**
         * @brief Removes events from all frame event slots.
         */
        void clearEvents();
    };

    /**
     * @brief Casts IAnimatable object to a specified type.
     * 
     * @tparam TInstance Type of the animated object. Usually it is a game object component.
     * 
     * @param [in] fromTarget   A pointer to the object implementing IAnimatable interface.
     * @return                  Casted pointer to the object or `nullptr` on casting failure.
     */
    template <typename TInstance>
    TInstance* getAnimatableTarget(const IAnimatable::Ptr& fromTarget)
    {
        if(fromTarget)
        {
            if(auto* target = fromTarget->getTarget(rtti::getTypeInfo<TInstance>()))
            {
                return static_cast<TInstance*>(target);
            }
        }

        return nullptr;
    }

    /**
     * @brief Retrieves the animated object from the animation state.
     * 
     * @param [in] state    State of the animation associated with the requested object.
     * @return              A pointer to the animated object.
     */
    template <typename TInstance>
    TInstance* getAnimatableTarget(AnimationState& state)
    {
        if(state.target)
        {
            if(auto* target = state.target->getTarget(rtti::getTypeInfo<TInstance>(), state.player.get()))
            {
                return static_cast<TInstance*>(target);
            }
        }

        return nullptr;
    }

    /**
     * @brief Provides basic functionality for controlling animation and keframe events.
     * 
     * Animation is a gradual change of a single parameter of an object (target, which usually is a game object component). 
     * This change is dictated by interpolation operation over a set of predefined parameter values at specified moments of time,
     * i.e. keyframes. 
     * 
     * This class encapsulates animation parameters that have been loaded from an animation asset and, perhaps, changed in animation editor. 
     * These parameters are keyframes and their associated data (values and events). They are shared among all animated objects 
     * which this animation is attached to. Individual settings like playback direction or animation weight are tuned via AnimationInstance which is individual
     * for each animated object.
     */
    class NAU_ANIMATION_EXPORT NAU_ABSTRACT_TYPE Animation : public virtual IRefCounted
    {
        NAU_CLASS(nau::animation::Animation, rtti::RCPolicy::StrictSingleThread, IRefCounted)

    public:

        /**
         * @brief Creates an animation player object.
         * 
         * @param [in] instance Animation instance to be played by the player.
         * @return              A pointer to the created player.
         */
        virtual IAnimationPlayer::Ptr createPlayer(AnimationInstance& instance) const;

        /**
         * @brief Animates the target according to the current animation state.
         * 
         * @param [in] frame            Current frame index.
         * @param [in] animationState   Animation state.
         */
        virtual void apply(int frame, AnimationState& animationState) const = 0;

        /**
         * @brief Retrieves the index of the last frame in the animation.
         * 
         * @return Index of the last frame.
         */
        virtual int getLastFrame() const = 0;

        /**
         * @brief Retrieves the animation duration in frames.
         * 
         * @return Animation duration in frames.
         */
        virtual float getDurationInFrames() const = 0;

        /**
         * @brief Retrieves a collection of events associated with the frame.
         * 
         * @param [in] frame    Index of the frame.
         * @return              A collection of the associated events.
         */
        const std::span<const FrameEvent> getEvents(int frame) const;

    protected:

        /**
         * @brief Adds the keyframe to the animation
         * 
         * @param [in] keyFrame A pointer to the keyframe to add.
         */
        virtual void addKeyFrame(const KeyFrame* keyFrame) = 0;

        /**
         * @brief Chronologically sorts keyframes event data.
         * 
         * See @ref m_perFrameData.
         */
        void sortFrames();

        /**
         * @brief Provides access to a data object for the specified frame. In case the object is default-initialized and added to the animation.
         * 
         * @param [in] frame    Index of the frame.
         * @return              Retrieved frame data.
         */
        Frame& getOrCreateFrameData(int frame);

        /**
         * @brief Provides access to a data object for the specified frame.
         *
         * @param [in] frame    Index of the frame.
         * @return              A pointer to the retrieved frame data or `nullptr` if **frame** was invalid.
         */
        Frame* getFrameData(int frame);

        /**
         * @brief Retrieves a corresponding data object for the specified frame. 
         *
         * @param [in] frame    Index of the frame.
         * @return              A pointer to the retrieved frame data or `nullptr` if **frame** was invalid.
         */
        const Frame* getFrameData(int frame) const;

        /**
         * @brief Serializes keyframes (event) data into the blk object.
         * 
         * @param [out] Blk object to write serialized data into.
         */
        void toBlk(DataBlock& blk) const;

        /**
         * @brief Deserializes keyframes (event) data from the blk object.
         *
         * @param [in] Blk object to extract data from.
         */
        void fromBlk(DataBlock& blk);

    protected:

        /**
         * @brief Stores event data associated with each keyframe.
         */
        std::vector<Frame> m_perFrameData;
    };

}  // namespace nau::animation
