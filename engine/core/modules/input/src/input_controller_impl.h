// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// input_system_impl.h

#pragma once
#include <gainput/gainput.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "nau/input_system.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service.h"

namespace nau
{
    class InputControllerImpl : public IInputController
    {
    public:
        InputControllerImpl(const std::string& name, IInputDevice* device) :
            m_name(name),
            m_device(device)
        {
        }

        std::string getName() const override;

        IInputDevice* getDevice() override;

        void update(float dt) override;

    private:
        std::string m_name;
        IInputDevice* m_device = nullptr;
    };
}  // namespace nau
