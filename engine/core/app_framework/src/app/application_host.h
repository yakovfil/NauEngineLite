// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once

#include "nau/async/task_base.h"
#include "nau/rtti/rtti_object.h"

namespace nau
{
    // Internal optional bridge. ApplicationImpl drives it after service startup.
    struct IApplicationHostStartup
    {
        NAU_TYPEID(nau::IApplicationHostStartup)
        virtual ~IApplicationHostStartup() = default;
        virtual async::Task<> startupApplicationHost() = 0;
    };
}  // namespace nau
