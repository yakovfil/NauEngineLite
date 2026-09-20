// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/app/application_init_delegate.h"

namespace nau
{
    struct BrowserRuntimeOptions
    {
        int foregroundProgressTimeoutMs = 10000;
    };

    // Call once on the PROXY_TO_PTHREAD application worker. The page receives
    // Module.nauRuntime.status and may call Module.nauRuntime.stop() asynchronously.
    // Returns zero after an orderly stop; retry always requires a page reload.
    int runBrowserApplication(ApplicationInitDelegate&, BrowserRuntimeOptions = {});
}  // namespace nau
