// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "disable_combo_state.h"

#include <memory>

#include "../button_state_animation.h"
#include "disable_state.h"
#include "disable_state_color.h"
#include "disable_state_size.h"

namespace nau::ui
{

    bool DisableComboState::initialize(nau::ui::NauButton* button, nau::ui::NauButtonData& data)
    {
        auto state = std::make_unique<DisableState>();
        if (state && state->initialize(button, data))
        {
            m_includedStates.push_back(std::move(state));
        }

        if (data.disabledAnimation)
        {
            auto stateAnimation = std::make_unique<ButtonStateDisabledAnimation>();
            if (stateAnimation->initialize(button, data))
            {
                m_includedStates.push_back(std::move(stateAnimation));
            }
        }

        auto stateColor = std::make_unique<DisableStateColor>();
        if (stateColor && stateColor->initialize(button, data))
        {
            m_includedStates.push_back(std::move(stateColor));
        }

        auto stateSize = std::make_unique<DisableStateSize>();
        if (stateSize && stateSize->initialize(button, data))
        {
            m_includedStates.push_back(std::move(stateSize));
        }

        return true;
    }

    void DisableComboState::enter(nau::ui::NauButton* button)
    {
        for (auto& state : m_includedStates)
        {
            state->enter(button);
        }
    }

    void DisableComboState::handleEvent(nau::ui::NauButton* button, nau::ui::EventType eventType)
    {
        for (auto& state : m_includedStates)
        {
            state->handleEvent(button, eventType);
        }
    }

    void DisableComboState::update(nau::ui::NauButton* button)
    {
        for (auto& state : m_includedStates)
        {
            state->update(button);
        }
    }

    void DisableComboState::exit(nau::ui::NauButton* button)
    {
        for (auto& state : m_includedStates)
        {
            state->exit(button);
        }
    }

} // namespace nau::ui