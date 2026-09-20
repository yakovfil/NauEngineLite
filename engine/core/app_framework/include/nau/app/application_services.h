// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/app/application.h"
#include "nau/app/application_init_delegate.h"
#include "nau/framework/framework_config.h"
#include "nau/utils/functor.h"
#include "nau/utils/result.h"

namespace nau
{
    // Creation and failure cleanup run on the calling thread. Browser callers
    // must use the application worker, never the page UI thread.
    Result<eastl::unique_ptr<Application>> createApplicationChecked(ApplicationInitDelegate&);

    eastl::unique_ptr<Application> createApplication(ApplicationInitDelegate&);

    [[deprecated("createApplication with delegate should by used")]]
    eastl::unique_ptr<Application> createApplication(Functor<Result<>()> preInitCallback = {});

    eastl::unique_ptr<IRttiObject> createPlatformWindowService();
}  // namespace nau
