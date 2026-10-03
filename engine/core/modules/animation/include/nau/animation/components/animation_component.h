// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <vector>

#include "nau/animation/controller/animation_controller.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/animation/playback/animation_transforms.h"
#include "nau/assets/asset_ref.h"
#include "nau/meta/class_info.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/components/scene_component.h"


namespace nau::animation
{
    struct AnimTrackCreationInfo final
    {
        NAU_CLASS_FIELDS(
            CLASS_FIELD(animationName),
            CLASS_FIELD(playMode),
            CLASS_FIELD(initialWeight),
            CLASS_FIELD(channelTargetPath),
            CLASS_FIELD(blendMethod),
            CLASS_FIELD(animationAsset))

        std::string animationName;        // keyframe + skeletal
        std::string playMode;             // keyframe + skeletal
        float initialWeight;              // keyframe + skeletal
        std::string channelTargetPath;    // keyframe
        std::string blendMethod;          // skeletal

        AnimationAssetRef animationAsset;  // keyframe + skeletal

        nau::WeakPtr<AnimationInstance> owningInstance;
    };

    /**
     * @brief Provides opportunity to animate properties of a target.
     */
    class NAU_ANIMATION_EXPORT AnimationComponent : public scene::SceneComponent,
                                                    public scene::IComponentUpdate,
                                                    public scene::IComponentEvents,
                                                    public ITransformAnimatable,
                                                    public scene::IComponentActivation

    {
        NAU_OBJECT(nau::animation::AnimationComponent,
                   scene::SceneComponent,
                   scene::IComponentUpdate,
                   scene::IComponentEvents,
                   ITransformAnimatable,
                   scene::IComponentActivation)

        NAU_DECLARE_DYNAMIC_OBJECT


        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Animation"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Animation (description)"))

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_animControllerType, "animControllerType"),
            CLASS_NAMED_FIELD(m_tracksCreationInfo, "tracksCreationInfo"))

    public:
        /**
         * @brief Default constructor.
         */
        AnimationComponent();

        /**
         * @brief Destructor.
         */
        ~AnimationComponent();

        /**
         * @brief Method that is called upon component restoration when the scene is being deserialized from an asset.
         */
        virtual void onAfterComponentRestored() override;

        /**
         * @brief Updates the animation in tick.
         *
         * @param [in] dt Time in seconds elapsed since the last update.
         */
        virtual void updateComponent(float dt) override;

        /**
         * @brief Binds the animation to the controller.
         *
         * @param [in] animation A pointer to the animation instance to bind.
         */
        void addAnimation(nau::Ptr<AnimationInstance> animation);

        /**
         * @brief Adds the object to the collection of the objects animated by the component.
         *
         * @param [in] target A pointer to the object to add.
         */
        void addAnimationTarget(IAnimationTarget::Ptr target);

        /**
         * @brief Adds the object to the collection of the objects animated by the component.
         *
         * @param [in] target A pointer to the object to add.
         * @param [in] player A pointer to the custom player object to use for the animation playback.
         */
        void addCustomAnimationTarget(IAnimationTarget::Ptr target, IAnimationPlayer::Ptr player);

        /**
         * @brief Assigns the controller to manage the animation instances attached to the component.
         *
         * @param [in] controller A pointer to the controller object to assign.
         */
        void setController(nau::Ptr<AnimationController> controller);

        /**
         * @brief Retrieves the controller managing the animation instances attached to the component.
         *
         * @return A pointer to the assigned controller or `NULL` if the controller is not attached.
         */
        AnimationController* getController() const;

        /**
         * @brief Retrieves the controller managing the animation instances attached to the component.
         *
         * @tparam TController Type of the controller to retrieve.
         *
         * @return A pointer to the assigned controller or `NULL` if the controller is not attached or if the TController mismatches the actual type of the asssigned controller.
         */
        template <class TController>
        TController* getController() const
        {
            if (auto* controller = getController(); controller && controller->is<TController>())
            {
                return controller->as<TController*>();
            }

            return nullptr;
        }

        /**
         * @brief Retrieves the component name.
         */
        const std::string& getName() const;

    private:
        AnimationController* getOrCreateController();

        virtual void animateTransform(const nau::math::Transform& transform) override;
        virtual void animateTranslation(const math::vec3& translation) override;
        virtual void animateRotation(const math::quat& rotation) override;
        virtual void animateScale(const math::vec3& scale) override;
        virtual void* getTarget(const rtti::TypeInfo& requestedTarget, IAnimationPlayer* player) override;
        virtual scene::SceneObject* getOwner() override;

    private:
        void applyTransform();
        void updateTrackSerializationInfo(nau::Ptr<AnimationInstance> animInstancePtr);

        async::Task<> activateComponentAsync() override;
        void deactivateComponent() override;

    private:
        struct AnimationTargetData
        {
            IAnimationTarget::Ptr target;
            IAnimationPlayer::Ptr player;

            AnimationTargetData(IAnimationTarget::Ptr&& inTarget, IAnimationPlayer::Ptr&& inPlayer) :
                target(std::move(inTarget)),
                player(std::move(inPlayer))
            {
            }
        };

        std::string m_animControllerType;
        std::vector<AnimTrackCreationInfo> m_tracksCreationInfo;

        nau::Ptr<AnimationController> m_controller;
        nau::math::Transform m_rootTransform;
        math::Transform m_frameTransform;
        std::vector<AnimationTargetData> m_targets;
        std::string m_name;
        TransformAnimationActionsFlag m_pendingTransforms = {};
    };
}  // namespace nau::animation
