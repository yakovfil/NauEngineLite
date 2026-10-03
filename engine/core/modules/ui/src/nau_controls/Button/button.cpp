// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/ui/button.h"

#include <cocos/base/CCDirector.h>

#include <memory>

#include "2d/CCCamera.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerMouse.h"
#include "nau/effects/node_animation.h"
#include "states/disable_state/disable_combo_state.h"
#include "states/disable_state/disable_state.h"
#include "states/disable_state/disable_state_color.h"
#include "states/disable_state/disable_state_size.h"
#include "states/hovered_state/hovered_combo_state.h"
#include "states/hovered_state/hovered_state.h"
#include "states/hovered_state/hovered_state_color.h"
#include "states/hovered_state/hovered_state_size.h"
#include "states/normal_state/normal_combo_state.h"
#include "states/normal_state/normal_state.h"
#include "states/normal_state/normal_state_color.h"
#include "states/normal_state/normal_state_size.h"
#include "states/pressed_state/pressed_combo_state.h"
#include "states/pressed_state/pressed_state.h"
#include "states/pressed_state/pressed_state_color.h"
#include "states/pressed_state/pressed_state_size.h"

namespace nau::ui
{
    namespace
    {
        class ButtonAnimator : public UiNodeAnimator
        {
            NAU_CLASS(ButtonAnimator, rtti::RCPolicy::StrictSingleThread, UiNodeAnimator)

        public:
            ButtonAnimator(NauButton& button)
                : UiNodeAnimator(button)
                , m_button(button)
            {
            }

            virtual void animateColor(const math::Color3& color) override
            {
                if (auto* sprite = m_button.getButtonSprite())
                {
                    auto intColor = e3dcolor(color);
                    sprite->setColor({ intColor.r, intColor.g, intColor.b });
                }
            }

            virtual void animateOpacity(float opacity) override
            {
                if (auto* sprite = m_button.getButtonSprite())
                {
                    uint8_t intOpacity = static_cast<uint8_t>(255.f * opacity);
                    sprite->setOpacity(intOpacity);
                }
            }

        private:
            NauButton& m_button;
        };
    }

NauButton::NauButton() :
    m_currentStateType(UIState::normal),
    m_sprite(nullptr),
    m_title(nullptr)
{
    setAnchorPoint(cocos2d::Vec2::ANCHOR_MIDDLE);
}

NauButton::~NauButton()
{
    m_stateCache[m_currentStateType]->exit(this);
    m_stateCache.clear();
}

NauButton* NauButton::create(
        NauButtonData& data)
{
    auto* btn = Node::create<NauButton>();
    if (btn && btn->init(data))
    {
        return btn;
    }

    CC_SAFE_DELETE(btn);
    return nullptr;
}

void NauButton::setOnClickCallback(OnClickCallback cb)
{
    m_onClick = cb;
}

Sprite* NauButton::getButtonSprite()
{
    return m_sprite;
}

const Sprite* NauButton::getButtonSprite() const
{
    return m_sprite;
}

void NauButton::InvokeClick()
{
    if(m_onClick)
    {
        m_onClick();
    }
}

bool NauButton::init(
        NauButtonData& data)
{
    if(!m_sprite)
    {
        m_sprite = nau::ui::Sprite::create();
        addChild(m_sprite);
    }

    m_stateCache[UIState::normal] = std::make_unique<NormalComboState>();
    m_stateCache[UIState::normal]->initialize(this, data);

    m_stateCache[nau::ui::UIState::hovered] = std::make_unique<HoveredComboState>();
    m_stateCache[nau::ui::UIState::hovered]->initialize(this, data);

    m_stateCache[nau::ui::UIState::pressed] = std::make_unique<PressedComboState>();
    m_stateCache[nau::ui::UIState::pressed]->initialize(this, data);

    m_stateCache[nau::ui::UIState::disabled] = std::make_unique<DisableComboState>();
    m_stateCache[nau::ui::UIState::disabled]->initialize(this, data);

    m_currentStateType = nau::ui::UIState::normal;
    m_stateCache[m_currentStateType]->enter(this);

    return true;
}

void NauButton::updateButtonData(NauButtonData& data)
{
    m_stateCache[m_currentStateType]->exit(this);
    m_stateCache.clear();

    init(data);
}

nau::Ptr<UiNodeAnimator> NauButton::createAnimator()
{
    return rtti::createInstance<ButtonAnimator>(*this);
}

void NauButton::handleEvent(EventType eventType)
{
    if (!m_interactable)
    {
        return;
    }

    if (m_stateCache[m_currentStateType])
    {
        m_stateCache[m_currentStateType]->handleEvent(this, eventType);
    }
}

void NauButton::changeState(UIState toState)
{
    if (m_currentStateType == toState)
    {
        return;
    }

    if (m_stateCache[m_currentStateType])
    {
        m_stateCache[m_currentStateType]->exit(this);
    }

    if (m_stateCache.find(toState) != m_stateCache.end()) 
    {
        m_currentStateType = toState;
    }
    else 
    {
        NAU_LOG_ERROR("Unknown button state!");
    }

    if (m_stateCache[m_currentStateType])
    {
        m_stateCache[m_currentStateType]->enter(this);
    }
}

void NauButton::update(float delta)
{
    if (m_stateCache[m_currentStateType])
    {
        m_stateCache[m_currentStateType]->update(this);
    }
}

void NauButton::setInteractable(bool interactable)
{
    UIControl::setInteractable(interactable);

    if (m_interactable)
    {
        changeState(UIState::normal);
    }
    else
    {
        changeState(UIState::disabled);
    }
}

void NauButton::setContentSize(const math::vec2& contentSize)
{
    Node::setContentSize(contentSize);

    if(m_sprite)
    {
        m_sprite->setContentSize(contentSize);
    }

    updateSpriteLocation();
    updateTitleLocation();
}

void NauButton::setTitleLabel(NauLabel* label)
{
    if (m_title != label)
    {
        if(m_title)
        {
            removeChild(m_title);
        }

        m_title = label;
        addChild(m_title);
        updateTitleLocation();
    }
}

NauLabel* NauButton::getTitleLabel()
{
    return m_title;
}

void NauButton::updateTitleLocation()
{
    if (m_title)
    {
        m_title->setPosition(_contentSize * 0.5f);
    }
}

void NauButton::updateSpriteLocation()
{
    if (m_sprite)
    {
        m_sprite->setPosition(_contentSize * 0.5f);
    }
}

}
