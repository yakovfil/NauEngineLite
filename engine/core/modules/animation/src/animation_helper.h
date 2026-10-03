// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/animation/controller/animation_controller.h"
#include "nau/animation/playback/animation_impl.h"

namespace nau
{
    class AsyncMessageStream;

    namespace scene
    {
        class SceneObject;
    }
}

namespace nau::animation
{
    class AnimationInstance;

    struct AnimationHelper final
    {
        static void broadcastFrameEvent(scene::SceneObject* owner, AnimationInstance* trackPlayer, const std::string& message);

        static nau::Ptr<AnimationController> createAnimationController(const std::string& className);
    };

    template <class TValue, class TKeyFrame>
    void findKeyFrames(const AnimationImpl<TValue>& animation, int frame, AnimationState& animationState, const TKeyFrame*& kfFrom, const TKeyFrame*& kfTo)
    {
        if (frame < 0)
        {
            NAU_LOG_WARNING("Invalid frame number {}", frame);
            return;
        }

        int targetKfIndex = animationState.baseKeyFrameIndex + 1;
        kfTo = animation.getKeyFrameAt(targetKfIndex);

        while(frame < animation.getLastFrame() && kfTo && kfTo->getFrame() <= frame)
        {
            if(targetKfIndex < animation.getNumKeyFrames() - 1)
            {
                ++targetKfIndex;
                kfTo = animation.getKeyFrameAt(targetKfIndex);
            }
            else
            {
                targetKfIndex = 1;
            }

            if(kfTo->getFrame() > frame)
            {
                animationState.baseKeyFrameIndex = targetKfIndex - 1;
                break;
            }
        }

        int baseKfIndex = targetKfIndex - 1;
        kfFrom = animation.getKeyFrameAt(baseKfIndex);

        while(frame < animation.getLastFrame() && kfFrom && kfFrom->getFrame() > frame)
        {
            if(baseKfIndex > 0)
            {
                --baseKfIndex;
                kfTo = kfFrom;
                kfFrom = animation.getKeyFrameAt(baseKfIndex);
            }
            else if (frame > kfFrom->getFrame())
            {
                baseKfIndex = animation.getNumKeyFrames() - 2;
            }
            else
            {
                animationState.baseKeyFrameIndex = baseKfIndex;
                break;
            }

            if(kfFrom->getFrame() <= frame)
            {
                animationState.baseKeyFrameIndex = baseKfIndex;
                break;
            }
        }

        if (animationState.isReversed)
        {
            std::swap(kfFrom, kfTo);
        }
    }
}  // namespace nau::animation
