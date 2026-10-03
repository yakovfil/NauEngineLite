#include <utility>
// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/app/platform_window.h

#pragma once
#include <utility>

#include "nau/rtti/rtti_object.h"

namespace nau
{
    struct NAU_ABSTRACT_TYPE IPlatformWindow : virtual IRefCounted
    {
        NAU_INTERFACE(nau::IPlatformWindow, IRefCounted)

        virtual void setVisible(bool) = 0;

        virtual bool isVisible() const = 0;

        virtual std::pair<unsigned, unsigned> getSize() const = 0;
        virtual void setSize(unsigned sizeX, unsigned sizeY) = 0;

        virtual std::pair<unsigned, unsigned> getClientSize() const = 0;

        virtual void setPosition(unsigned positionX, unsigned positionY) = 0;
        virtual std::pair<unsigned, unsigned> getPosition() const = 0;

        virtual void setName(const char* name) = 0;
    };

}  // namespace nau
