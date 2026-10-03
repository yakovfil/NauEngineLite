// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <functional>
#include <unordered_map>
#include <memory>

#include "button_data.h"
#include "label.h"
#include "nau/ui/elements/sprite.h"
#include "nau/ui/ui_control.h"


namespace nau::ui
{
    class ButtonStateBase;

    /**
     * @brief Enumerates button animation targets.
     */
    enum class ButtonTransition 
    {
        sprite,
        color,
        size
    };

/**
 * @brief Encapsulates GUI button logic and data.
 * 
 * A button is a container storing a sprite or a text label providing means to custom handle various cursor events.
 */
class NAU_UI_EXPORT NauButton : public UIControl
{
public:

    /**
     * @brief Functor type that is called when the button is clicked on.
     */
    using OnClickCallback = std::function<void()>;

    /**
     * @brief Default constructor.
     */
    NauButton();

    /**
     * @brief Destructor.
     */
    virtual ~NauButton();


    /**
     * @brief Creates a button object.
     * 
     * @return A pointer to the created button object.
     */
    static NauButton* create(
        NauButtonData& data);

    
    /**
     * @brief Changes the callback that is dispatched when the button is clicked.
     */
    void setOnClickCallback(OnClickCallback cb);
    
    /**
     * @brief Retrieves the button sprite.
     * 
     * @return A pointer to the button sprite.
     */
    Sprite* getButtonSprite();
    
    /**
     * @brief Retrieves the button sprite.
     *
     * @return A pointer to the button sprite.
     */
    const Sprite* getButtonSprite() const;

    /**
     * @brief Changes the button state.
     * 
     * @param [in] toState State to transition the button to.
     */
    void changeState(UIState toState);

    /**
     * @brief This function is called when any cursor event is triggered.
     *
     * @param [in] eventType Type of the triggered event.
     */
    virtual void handleEvent(EventType eventType) override;

    virtual void update(float delta) override;

    /**
     * @brief Changes whether the button can be interacted with (i.e. whether it can trigger cursor events).
     *
     * @param [in] interactable  Indicates whether the button should be interactable.
     */
    virtual void setInteractable(bool interactable) override;

    /**
     * @brief Changes the button size.
     * 
     * @param [in] contentSize Size to assign.
     */
    virtual void setContentSize(const math::vec2& contentSize) override;
    
    /**
     * @brief Changes the title label of the button.
     * 
     * @param [in] title Label to assign
     */
    void setTitleLabel(NauLabel* title);
    
    /**
     * @brief Retrieves the label that contains the button title.
     * 
     * @return Button title label.
     */
    NauLabel* getTitleLabel();

    /**
     * @brief Aligns the button sprite by the button center.
     */
    void updateSpriteLocation();

    /**
     * @brief Aligns the button title label by the button center.
     */
    void updateTitleLocation();

    /**
     * @brief Changes button data.
     * 
     * @param [in] data Value to set.
     */
    void updateButtonData(NauButtonData& data);

protected:
    OnClickCallback m_onClick;
    bool init(NauButtonData& data);
    virtual nau::Ptr<UiNodeAnimator> createAnimator() override;

private:
    Sprite* m_sprite {nullptr};
    NauLabel* m_title {nullptr};

    ButtonTransition m_transitionType {ButtonTransition::sprite};
    UIState m_currentStateType {UIState::normal};
    std::unordered_map<UIState, std::unique_ptr<ButtonStateBase>> m_stateCache{};

    void InvokeClick();

    friend class PressedComboState;
};
}
