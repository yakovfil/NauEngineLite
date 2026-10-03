// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "button_state_animation.h"

#include <string>

#include "nau/animation/components/animation_component.h"
#include "nau/animation/controller/animation_controller.h"
#include "nau/ui/button.h"
#include "nau/ui/button_data.h"

namespace nau::ui
{
    namespace
    {
        constexpr char STATE_NAME_NORMAL[] = "normal";
        constexpr char STATE_NAME_HOVERED[] = "hovered";
        constexpr char STATE_NAME_PRESSED[] = "pressed";
        constexpr char STATE_NAME_DISABLED[] = "disabled";
    }

    ButtonStateAnimation::ButtonStateAnimation(const char* name)
        : m_name(name)
    {
    }

    bool ButtonStateAnimation::initialize(nau::ui::NauButton* button, nau::ui::NauButtonData& data)
    {
        if (auto* animComp = button->getAnimationComponent())
        {
            if (auto stateAnimInstance = createStateAnimation(data, m_name.c_str(), m_reverseAnimationOnExit))
            {
                animComp->getController()->addAnimation(stateAnimInstance);
                animComp->addCustomAnimationTarget(button->getAnimator(), stateAnimInstance->getPlayer());

                return true;
            }
        }

        return false;
    }

    void ButtonStateAnimation::enter(nau::ui::NauButton* button)
    {
        if (auto* animComp = button->getAnimationComponent())
        {
            if (auto* animController = animComp->getController())
            {
                if (auto* animInstance = animController->getAnimInstance(getName()))
                {
                    if (m_reverseAnimationOnExit)
                    {
                        animInstance->getPlayer()->reverse(false);
                    }
                    animInstance->getPlayer()->play();
                }
            }
        }
    }

    void ButtonStateAnimation::exit(nau::ui::NauButton* button)
    {
        if (auto* animComp = button->getAnimationComponent())
        {
            if (auto* animController = animComp->getController())
            {
                if (auto* animInstance = animController->getAnimInstance(getName()))
                {
                    if (m_reverseAnimationOnExit)
                    {
                        animInstance->getPlayer()->reverse(true);
                        animInstance->getPlayer()->play();
                    }
                    else
                    {
                        animInstance->getPlayer()->stop();
                    }
                }
            }
        }
    }

    const std::string& ButtonStateAnimation::getName() const
    {
        return m_name;
    }

    ButtonStateNormalAnimation::ButtonStateNormalAnimation()
        : ButtonStateAnimation(STATE_NAME_NORMAL)
    { }

    nau::Ptr<animation::AnimationInstance> ButtonStateNormalAnimation::createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const
    {
        if (data.normalAnimation)
        {
            reverseOnExit = data.normalAnimation.bPlayReversedOnExit;
            return rtti::createInstance<animation::AnimationInstance>(getName(), *data.normalAnimation.animation);
        }

        return nullptr;
    }

    ButtonStateHoveredAnimation::ButtonStateHoveredAnimation()
        : ButtonStateAnimation(STATE_NAME_HOVERED)
    { }

    nau::Ptr<animation::AnimationInstance> ButtonStateHoveredAnimation::createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const
    {
        if (data.hoveredAnimation)
        {
            reverseOnExit = data.hoveredAnimation.bPlayReversedOnExit;
            return rtti::createInstance<animation::AnimationInstance>(getName(), *data.hoveredAnimation.animation);
        }

        return nullptr;
    }

    ButtonStatePressedAnimation::ButtonStatePressedAnimation()
        : ButtonStateAnimation(STATE_NAME_PRESSED)
    { }

    nau::Ptr<animation::AnimationInstance> ButtonStatePressedAnimation::createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const
    {
        if (data.clickedAnimation)
        {
            reverseOnExit = data.clickedAnimation.bPlayReversedOnExit;
            return rtti::createInstance<animation::AnimationInstance>(getName(), *data.clickedAnimation.animation);
        }

        return nullptr;
    }

    ButtonStateDisabledAnimation::ButtonStateDisabledAnimation()
        : ButtonStateAnimation(STATE_NAME_DISABLED)
    { }

    nau::Ptr<animation::AnimationInstance> ButtonStateDisabledAnimation::createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const
    {
        if (data.disabledAnimation)
        {
            reverseOnExit = data.disabledAnimation.bPlayReversedOnExit;
            return rtti::createInstance<animation::AnimationInstance>(getName(), *data.disabledAnimation.animation);
        }

        return nullptr;
    }
} // namespace nau::ui