// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <chrono>

#include "nau/diag/error.h"

namespace nau
{
    enum class ApplicationPhase
    {
        Created,
        PreInitializing,
        Initializing,
        Running,
        Stopping,
        Stopped
    };

    // Except for Application::stop(), call this interface on the owning thread.
    // pollLifecycle advances startup/cleanup and executor work without game updates.
    struct ApplicationLifecycle
    {
        NAU_TYPEID(nau::ApplicationLifecycle)
        virtual ~ApplicationLifecycle() = default;
        virtual void beginStartup() = 0;
        virtual bool pollLifecycle() = 0;
        virtual ApplicationPhase getPhase() const = 0;
        virtual Error::Ptr getLifecycleError() const = 0;
        // Opaque change token for completed service work, not a task count.
        virtual uint64_t getLifecycleProgress() const = 0;
        virtual bool stepWithElapsedTime(std::chrono::milliseconds elapsed) = 0;
    };
}  // namespace nau
