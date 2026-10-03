// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "normal_combo_state.h"

#include <memory>

#include "../button_state_animation.h"
#include "normal_state.h"
#include "normal_state_color.h"
#include "normal_state_size.h"

namespace nau::ui
{
    bool NormalComboState::initialize(nau::ui::NauButton* button, nau::ui::NauButtonData& data)
    {
        auto state = std::make_unique<NormalState>();
        if (state && state->initialize(button, data))
        {
            m_includedStates.push_back(std::move(state));
        }

        if (data.normalAnimation)
        {
            auto stateAnimation = std::make_unique<ButtonStateNormalAnimation>();
            if (stateAnimation->initialize(button, data))
            {
                m_includedStates.push_back(std::move(stateAnimation));
            }
        }

        auto stateColor = std::make_unique<NormalStateColor>();
        if (stateColor && stateColor->initialize(button, data))
        {
            m_includedStates.push_back(std::move(stateColor));
        }

        auto stateSize = std::make_unique<NormalStateSize>();
        if (stateSize && stateSize->initialize(button, data))
        {
            m_includedStates.push_back(std::move(stateSize));
        }

        return true;
    }

    void NormalComboState::enter(nau::ui::NauButton* button)
    {
        for (auto& state : m_includedStates)
        {
            state->enter(button);
        }
    }

    void NormalComboState::handleEvent(nau::ui::NauButton* button, nau::ui::EventType eventType)
    {
        for (auto& state : m_includedStates)
        {
            state->handleEvent(button, eventType);
        }
    }

    void NormalComboState::update(nau::ui::NauButton* button)
    {
        for (auto& state : m_includedStates)
        {
            state->update(button);
        }
    }

    void NormalComboState::exit(nau::ui::NauButton* button)
    {
        for (auto& state : m_includedStates)
        {
            state->exit(button);
        }
    }

} // namespace nau::ui