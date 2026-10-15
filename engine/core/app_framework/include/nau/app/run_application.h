// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include "nau/app/application_delegate.h"

namespace nau
{
    enum class ApplicationScheduling
    {
        // Native loop policy; browser updates as fast as its event loop allows.
        Default,
        Presentation
    };

    struct ApplicationHostOptions
    {
        ApplicationScheduling scheduling = ApplicationScheduling::Default;
        int foregroundProgressTimeoutMs = 10000;
    };

    // Owns the delegate until application teardown. Native calls return the final
    // exit code; cooperative browser calls hand control to the browser scheduler.
    int runApplication(ApplicationDelegate::Ptr = nullptr);
    int runApplication(ApplicationDelegate::Ptr, ApplicationHostOptions);
}  // namespace nau
