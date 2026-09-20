// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "browser_window_manager.h"
#include "nau/module/module.h"
#include "nau/service/service_provider.h"

namespace nau
{
    class BrowserPlatformModule final : public DefaultModuleImpl
    {
        void initialize() override
        {
            NAU_MODULE_EXPORT_CLASS(nau::BrowserWindowManager);
        }
    };
}  // namespace nau
IMPLEMENT_MODULE(nau::BrowserPlatformModule)
