// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <gainput/gainput.h>

#include <vector>

#include "nau/input.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"
#include "nau/service/service.h"

namespace nau::input
{
    struct NAU_ABSTRACT_TYPE GainputAccess
    {
        NAU_TYPEID(nau::input::GainputAccess)

        virtual gainput::InputManager& getGainput() = 0;
    };

    struct NAU_ABSTRACT_TYPE InputManager
    {
        NAU_TYPEID(nau::input::InputManager)

        virtual void setScreenResolution(int x, int y) = 0;
        virtual void update() = 0;
        virtual void update(float dt) = 0;
        virtual bool isKeyboardButtonPressed(int deviceId, nau::input::Key key) = 0;
        virtual bool isKeyboardButtonHold(int deviceId, nau::input::Key key) = 0;
        virtual bool isMouseButtonPressed(int deviceId, nau::input::MouseKey key) = 0;
        virtual bool isMouseButtonReleased(int deviceId, nau::input::MouseKey key) = 0;
        virtual bool isMouseButtonHold(int deviceId, nau::input::MouseKey key) = 0;
        virtual float getMouseAxisValue(int deviceId, nau::input::MouseKey axis) = 0;
        virtual float getMouseAxisDelta(int deviceId, nau::input::MouseKey axis) = 0;
    };

    class InputManagerImpl final : public IServiceInitialization,
                                   public GainputAccess,
                                   public InputManager
    {
        NAU_RTTI_CLASS(nau::input::InputManagerImpl, IServiceInitialization, GainputAccess, InputManager)

    public:
        virtual async::Task<> preInitService() override;
        virtual async::Task<> initService() override;

        virtual gainput::InputManager& getGainput() override;
        virtual void setScreenResolution(int x, int y) override;
        virtual void update() override;
        virtual void update(float dt) override;
        virtual bool isKeyboardButtonPressed(int deviceId, nau::input::Key key) override;
        virtual bool isKeyboardButtonHold(int deviceId, nau::input::Key key) override;
        virtual bool isMouseButtonPressed(int deviceId, nau::input::MouseKey key) override;
        virtual bool isMouseButtonReleased(int deviceId, nau::input::MouseKey key) override;
        virtual bool isMouseButtonHold(int deviceId, nau::input::MouseKey key) override;
        virtual float getMouseAxisValue(int deviceId, nau::input::MouseKey axis) override;
        virtual float getMouseAxisDelta(int deviceId, nau::input::MouseKey axis) override;

    private:
        bool inited = false;
        gainput::InputManager m_inputManager = {false};
        gainput::InputMap m_inputMap = {m_inputManager};
        gainput::DeviceId m_keyboard;
        gainput::DeviceId m_mouse;
    };
}  // namespace nau::input
