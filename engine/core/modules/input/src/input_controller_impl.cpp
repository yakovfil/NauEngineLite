// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "input_controller_impl.h"

#include <string>

namespace nau
{
    std::string InputControllerImpl::getName() const
    {
        return m_name;
    }

    IInputDevice* InputControllerImpl::getDevice()
    {
        return m_device;
    }

    void InputControllerImpl::update(float dt)
    {
    }

}  // namespace nau
