// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <string_view>

#include "nau/animation/interfaces/animatable.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/async/task.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/scene/nau_object.h"

namespace nau::animation
{
    class IAnimatable;

    /**
     * @brief Manages multiple animation instances animating one target.
     * 
     * @note No blending is performed. For a blending controller see BlendAnimationController.
     */
    class NAU_ANIMATION_EXPORT NAU_ABSTRACT_TYPE AnimationController : public virtual IRefCounted
    {
        NAU_CLASS(nau::animation::AnimationController, rtti::RCPolicy::StrictSingleThread, IRefCounted)

        using Ptr = nau::Ptr<AnimationController>;

    public:
        static constexpr float NEGLIGIBLE_WEIGHT = 5.e-3f;

        /**
         * @brief Encapsulates a handle to an animation instance.
         */
        struct NAU_ANIMATION_EXPORT AnimInstanceDescriptor
        {
            AnimInstanceDescriptor() = default;
            AnimInstanceDescriptor(const AnimationInstance& animInstance);
            AnimInstanceDescriptor(nau::string animName);
            AnimInstanceDescriptor(const std::string& animName);

            friend bool operator==(const AnimInstanceDescriptor&, const AnimInstanceDescriptor&) = default;

            // todo: hash it
            std::string name;
        };

        using TAnimDescr = AnimInstanceDescriptor;
        using TAnimDescrParam = const TAnimDescr&;


        /**
         * @brief Default constructor.
         */
        AnimationController();

        /**
         * @brief Destructor.
         */
        ~AnimationController();

        /**
         * @brief Loads managed animations from assets.
         */
        async::Task<> load();

        /**
         * @brief Advances each managed animation.
         * 
         * @param [in] dt       Time in seconds elapsed since the last update.
         * @param [in] target   Target object to apply animations to.
         */
        virtual void update(float dt, const IAnimatable::Ptr& target);

        /**
         * @brief Binds the animation to the controller.
         * 
         * @param [in] animation Animation to manage.
         */
        virtual void addAnimation(nau::Ptr<AnimationInstance> animation);

        /**
         * @brief Retrieves the animation frame rate.
         * 
         * @return Animation frame rate (frames per second).
         */
        float getFrameRate() const;

        /**
         * @brief Retrieves the total number of frames accumulated from the controller updates.
         *
         * @return Total number of frames.
         * 
         * @note    This count is not nullified upon any animation reaches its end. 
         *          In order to retrieve the current frame of a concrete animation instance, 
         *          address to the corresponding AnimationInstance methods.
         */
        int getCurrentFrame() const;

        /**
         * @brief Retrieves the weight of a managed animation.
         * 
         * @param [in] animationId A handle to the animation instance to retrieve the weight of.
         * @return Animation instance weight.
         */
        virtual float getWeight(TAnimDescrParam animationId) const;

        /**
         * @brief Retrieves the number of animation instances managed by the controller.
         * 
         * @return Number of managed animation instances.
         */
        int getAnimationInstancesCount() const;

        /**
         * @brief Retrieves managed animation instance.
         * 
         * @param [in] index    Index of the animation instance to retrieve.
         * @return              A pointer to the managed animation instance object.
         */
        AnimationInstance* getAnimationInstanceAt(int index);

        /**
         * @brief Retrieves managed animation instance.
         *
         * @param [in] animationId  A handle to the animation instance to retrieve.
         * @return                  A pointer to the managed animation instance object.
         */
        AnimationInstance* getAnimInstance(TAnimDescrParam animationId);

        virtual const std::string_view getControllerTypeName() const = 0;

    protected:
        virtual void onLoaded() { }

    private:

        /**
         * @brief A collection of managed animation instances.
         */
        std::vector<nau::Ptr<AnimationInstance>> m_animations;
        int m_frame = 0;
        float m_frameTime = .0f;
    };

}  // namespace nau::animation
