// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <optional>

#include "EASTL/chrono.h"
#include "nau/async/executor.h"
#include "nau/async/task.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/ptr.h"
#include "nau/runtime/async_disposable.h"
#include "nau/runtime/disposable.h"

namespace nau
{
    struct NAU_ABSTRACT_TYPE WorkQueue : async::Executor
    {
        NAU_INTERFACE(nau::WorkQueue, async::Executor)

        using Ptr = nau::Ptr<WorkQueue>;

        NAU_KERNEL_EXPORT static WorkQueue::Ptr create();

        virtual async::Task<> waitForWork() = 0;

        virtual void poll(std::optional<eastl::chrono::milliseconds> time = eastl::chrono::milliseconds{0}) = 0;

        virtual void notify() = 0;

        // A non-blocking notification hook; it must not re-enter this queue.
        virtual void setWakeCallback(void (*callback)()) = 0;

        virtual void setName(std::string name) = 0;

        virtual std::string getName() const = 0;
    };

}  // namespace nau
