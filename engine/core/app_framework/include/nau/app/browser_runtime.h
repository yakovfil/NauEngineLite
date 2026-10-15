// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/app/application_init_delegate.h"
#include "nau/app/application_runtime_service.h"

namespace nau
{
    // Optional owner-thread services (for example graphics) polled at bounded
    // lifecycle intervals, including startup and cleanup, without game steps.
    struct IBrowserRuntimeService : IApplicationRuntimeService
    {
        NAU_TYPEID(nau::IBrowserRuntimeService)
        NAU_CLASS_BASE(IApplicationRuntimeService)
        virtual ~IBrowserRuntimeService() = default;
        virtual Result<> pollBrowserRuntime() = 0;
        Result<> pollApplicationRuntime() final
        {
            return pollBrowserRuntime();
        }
    };

    struct BrowserRuntimeOptions
    {
        int foregroundProgressTimeoutMs = 10000;
        // Opt-in presentation mode returns to the worker event loop between polls.
        // The delegate must have static or otherwise persistent lifetime.
        bool cooperativePresentation = false;
        // Step the application on each presentation callback using elapsed time.
        // Requires cooperativePresentation; other callers keep the fixed cadence.
        bool frameDrivenUpdates = false;
        // Cooperative updates without an engine-imposed frame delay. The owning
        // application host uses this for ApplicationScheduling::Default.
        bool uncappedUpdates = false;
    };

    // Call once on the PROXY_TO_PTHREAD application worker. The page receives
    // Module.nauRuntime.status and may call Module.nauRuntime.stop() asynchronously.
    // Returns zero after an orderly stop; retry always requires a page reload.
    int runBrowserApplication(ApplicationInitDelegate&, BrowserRuntimeOptions = {});
}  // namespace nau
