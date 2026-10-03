// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "button_state_base.h"
#include "nau/rtti/ptr.h"

namespace nau::animation
{
    class AnimationController;
    class AnimationInstance;
}

namespace nau::ui
{
    class NauButton;
    struct NauButtonData;

    class ButtonStateAnimation : public ButtonStateBase
    {
    protected:
        ButtonStateAnimation(const char* name);

    public:
        virtual bool initialize(nau::ui::NauButton* button, nau::ui::NauButtonData& data) override;
        virtual void enter(nau::ui::NauButton* button) override;
        virtual void exit(nau::ui::NauButton* button) override;

        const std::string& getName() const;

    protected:
        virtual nau::Ptr<animation::AnimationInstance> createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const = 0;

    private:
        std::string m_name;
        bool m_reverseAnimationOnExit = false;
    };

    class ButtonStateNormalAnimation : public ButtonStateAnimation
    {
    public:
        ButtonStateNormalAnimation();

        virtual nau::Ptr<animation::AnimationInstance> createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const override;
    };

    class ButtonStateHoveredAnimation : public ButtonStateAnimation
    {
    public:
        ButtonStateHoveredAnimation();

        virtual nau::Ptr<animation::AnimationInstance> createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const override;
    };

    class ButtonStatePressedAnimation : public ButtonStateAnimation
    {
    public:
        ButtonStatePressedAnimation();

        virtual nau::Ptr<animation::AnimationInstance> createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const override;
    };

    class ButtonStateDisabledAnimation : public ButtonStateAnimation
    {
    public:
        ButtonStateDisabledAnimation();

        virtual nau::Ptr<animation::AnimationInstance> createStateAnimation(const NauButtonData& data, const char* name, bool& reverseOnExit) const override;
    };

} // namespace nau::ui
