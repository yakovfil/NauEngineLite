// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/runtime/runtime_state.h

#pragma once

#include <memory>

#include "nau/kernel/kernel_config.h"
#include "nau/utils/functor.h"

namespace nau
{
    struct RuntimeState
    {
        using Ptr = std::unique_ptr<RuntimeState>;

        NAU_KERNEL_EXPORT
        static RuntimeState::Ptr create();

        virtual ~RuntimeState() = default;

        virtual Functor<bool()> shutdown(bool doCompleteShutdown = true) = 0;

        virtual void completeShutdown() = 0;
    };
}  // namespace nau
