// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_devices_impl.h"

#include <string>
#include <string_view>

namespace nau
{
    GAKeyboardDevice::GAKeyboardDevice(gainput::InputManager& inputManager) :
        m_inputManager(inputManager)
    {
        m_id = m_inputManager.CreateDevice<gainput::InputDeviceKeyboard>();
    }

    std::string GAKeyboardDevice::getName() const
    {
        return "keyboard";
    }

    IInputDevice::Type GAKeyboardDevice::getType() const
    {
        return Type::Keyboard;
    }

    unsigned GAKeyboardDevice::getKeysNum() const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            auto* state = device->GetInputState();
            if (state != nullptr)
            {
                return state->GetButtonCount();
            }
        }
        return 0;
    }

    unsigned GAKeyboardDevice::getAxisNum() const
    {
        return 0;
    }

    std::string GAKeyboardDevice::getKeyName(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        char buffer[64];
        device->GetButtonName(keyId, buffer, 63);
        return buffer;
    }

    std::string GAKeyboardDevice::getAxisName(unsigned axisId) const
    {
        return "";
    }

    unsigned GAKeyboardDevice::getKeyByName(const std::string_view keyName) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            return device->GetButtonByName(keyName.data());
        }
        return UINT_MAX;
    }

    unsigned GAKeyboardDevice::getAxisByName(const std::string_view axisName) const
    {
        return UINT_MAX;
    }

    IInputDevice::KeyState GAKeyboardDevice::getKeyState(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            if (device->GetBool(keyId))
            {
                return KeyState::Pressed;
            }
        }
        return KeyState::Released;
    }

    float GAKeyboardDevice::getAxisState(unsigned axisId) const
    {
        return 0.f;
    }

    GAMouseDevice::GAMouseDevice(gainput::InputManager& inputManager) :
        m_inputManager(inputManager)
    {
        m_id = m_inputManager.CreateDevice<gainput::InputDeviceMouse>();
    }

    std::string GAMouseDevice::getName() const
    {
        return "mouse";
    }

    IInputDevice::Type GAMouseDevice::getType() const
    {
        return Type::Mouse;
    }

    unsigned GAMouseDevice::getKeysNum() const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            auto* state = device->GetInputState();
            if (state != nullptr)
            {
                return state->GetButtonCount();
            }
        }
        return 0;
    }

    unsigned GAMouseDevice::getAxisNum() const
    {
        return 3;
    }

    std::string GAMouseDevice::getKeyName(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        char buffer[64];
        device->GetButtonName(keyId, buffer, 63);
        return buffer;
    }

    std::string GAMouseDevice::getAxisName(unsigned axisId) const
    {
        return "";
    }

    unsigned GAMouseDevice::getKeyByName(const std::string_view keyName) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        return device->GetButtonByName(keyName.data());
    }

    unsigned GAMouseDevice::getAxisByName(const std::string_view axisName) const
    {
        return UINT_MAX;
    }

    IInputDevice::KeyState GAMouseDevice::getKeyState(unsigned keyId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            if (device->GetBool(keyId))
            {
                return KeyState::Pressed;
            }
        }
        return KeyState::Released;
    }

    float GAMouseDevice::getAxisState(unsigned axisId) const
    {
        auto* device = m_inputManager.GetDevice(m_id);
        if (device != nullptr)
        {
            return device->GetFloat(gainput::MouseAxisX + axisId);
        }
        return 0.f;
    }
}  // namespace nau