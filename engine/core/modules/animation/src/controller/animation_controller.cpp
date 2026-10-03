// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/animation/controller/animation_controller.h"

#include <algorithm>
#include <string>

#include "nau/animation/playback/animation_instance.h"

namespace nau::animation
{
    AnimationController::AnimInstanceDescriptor::AnimInstanceDescriptor(const AnimationInstance& animInstance)
        : name(animInstance.getName())
    {
    }

    AnimationController::AnimInstanceDescriptor::AnimInstanceDescriptor(nau::string animName)
        : name(animName.tostring())
    {
    }

    AnimationController::AnimInstanceDescriptor::AnimInstanceDescriptor(const std::string& animName) :
        name(animName)
    {
    }

    AnimationController::AnimationController() = default;

    AnimationController::~AnimationController() = default;
    
    async::Task<> AnimationController::load()
    {
        for(auto animation : m_animations)
        {
            co_await animation->load();
        }

        onLoaded();
    }

    void AnimationController::update(float dt, const IAnimatable::Ptr& target)
    {
        for(auto animation : m_animations)
        {
            animation->update(*this, dt, target);
        }

        m_frameTime += dt;

        if (m_frameTime >= 1.f / getFrameRate())
        {
            m_frameTime -= 1.f / getFrameRate();
            ++m_frame;
        }
    }

    void AnimationController::addAnimation(nau::Ptr<AnimationInstance> animation)
    {
        m_animations.push_back(animation);
    }
    
    float AnimationController::getFrameRate() const
    {
        return 60.f;
    }

    int AnimationController::getCurrentFrame() const
    {
        return m_frame;
    }

    int AnimationController::getAnimationInstancesCount() const
    {
        return m_animations.size();
    }

    AnimationInstance* AnimationController::getAnimationInstanceAt(int index) 
    {
        if(0 <= index && index < m_animations.size())
        {
            return m_animations[index].get();
        }

        return nullptr;
    }

    AnimationInstance* AnimationController::getAnimInstance(TAnimDescrParam animationId)
    {
        auto it = std::find_if(m_animations.begin(), m_animations.end(), [animationId](const auto& animInstancePtr)
        {
            return *animInstancePtr.get() == animationId;
        });

        return it != m_animations.end() ? it->get() : nullptr;
    }

    float AnimationController::getWeight(TAnimDescrParam id) const
    {
        return 1.f;
    }

}  // namespace nau::animation
