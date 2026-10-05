// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "nau/app/run_application.h"

#include <cstdio>

#include "nau/app/application_lifecycle.h"
#include "nau/app/application_services.h"

namespace nau
{
    int runApplication(ApplicationDelegate::Ptr delegate)
    {
        return runApplication(std::move(delegate), {});
    }

    int runApplication(ApplicationDelegate::Ptr delegate, ApplicationHostOptions)
    {
        if (!delegate)
            return 1;
        auto created = createApplicationChecked(*delegate);
        if (!created)
        {
            std::fprintf(stderr, "%s\n", created.getError()->getDiagMessage().c_str());
            return 1;
        }
        auto app = std::move(*created);
        auto& lifecycle = app->as<ApplicationLifecycle&>();
        lifecycle.beginStartup();
        while (app->step())
        {
        }
        auto error = lifecycle.getLifecycleError();
        if (error)
            std::fprintf(stderr, "%s\n", error->getDiagMessage().c_str());
        app.reset();
        return error ? 1 : 0;
    }
}  // namespace nau
