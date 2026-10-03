// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "animation_helper.h"

#include <string>

#include "nau/animation/controller/animation_controller_blend.h"
#include "nau/animation/controller/animation_controller_direct.h"
#include "nau/animation/data/events.h"
#include "nau/animation/playback/animation_instance.h"
#include "nau/animation/playback/animation_skeleton.h"
#include "nau/messaging/messaging.h"
#include "nau/scene/scene_object.h"

namespace nau::animation
{

    void AnimationHelper::broadcastFrameEvent(scene::SceneObject* owner, AnimationInstance* trackPlayer, const std::string& message)
    {
        if (owner && trackPlayer)
        { 
            auto& broadcaster = owner->getMessageSource();

            events::FrameEventData eventData
            {
                trackPlayer->getName(),
                message
            };

            events::AnimTrackPlaybackEvent.post(broadcaster, eventData);
        }
    }

    nau::Ptr<AnimationController> AnimationHelper::createAnimationController(const std::string& className)
    {
        if (className == "direct")
        {
            return rtti::createInstance<DirectAnimationController>();
        }
        else if (className == "blend_skeletal")
        {
            nau::Ptr<nau::animation::AnimationMixer> animMixer = rtti::createInstance<nau::animation::SkeletalAnimationMixer>().get();
            nau::Ptr<nau::animation::BlendAnimationController> blendController = rtti::createInstance<nau::animation::BlendAnimationController>(animMixer);
            return blendController;
        }

        NAU_ASSERT(false, "Unknown Animation Controller Class: {}", className);
        return {};
    }
} // namespace nau::animation