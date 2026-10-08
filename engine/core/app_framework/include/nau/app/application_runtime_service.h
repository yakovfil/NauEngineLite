// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once

#include "nau/rtti/type_info.h"
#include "nau/utils/result.h"

namespace nau
{
    // Optional owner-thread progress, independent of game update eligibility.
    // A registered service must tolerate polls before initialization completes
    // and through its own cleanup, returning a no-op after its work settles.
    // Once Stop is observed it must not start ordinary application work.
    struct IApplicationRuntimeService
    {
        NAU_TYPEID(nau::IApplicationRuntimeService)
        virtual ~IApplicationRuntimeService() = default;
        virtual Result<> pollApplicationRuntime() = 0;
    };
}  // namespace nau
