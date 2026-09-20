// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <cstdio>

#include "nau/app/application_lifecycle.h"
#include "nau/app/application_services.h"
#include "nau/module/module_manager.h"

#ifdef __EMSCRIPTEN__
    #include "nau/app/browser_runtime.h"
#endif

namespace
{
    class MinimalApplicationDelegate final : public nau::ApplicationInitDelegate
    {
        nau::Result<> configureApplication() override
        {
            return {};
        }

        nau::Result<> initializeApplication() override
        {
            NauCheckResult(nau::loadModulesList(NAU_MODULES_LIST))
            nau::getServiceProvider().addService(nau::createPlatformWindowService());
            return {};
        }
    };
}  // namespace

int main(int argc, char** argv)
{
    using namespace nau;
    using namespace std::chrono_literals;

    MinimalApplicationDelegate delegate;
#ifdef __EMSCRIPTEN__
    return runBrowserApplication(delegate);
#else
    auto creation = createApplicationChecked(delegate);
    if (!creation)
    {
        std::fprintf(stderr, "%s\n", creation.getError()->getDiagMessage().c_str());
        return 1;
    }
    auto app = std::move(*creation);
    app->startupOnCurrentThread();

    while (app->step())
    {
        std::this_thread::sleep_for(100ms);
    }

    if (auto error = app->as<ApplicationLifecycle&>().getLifecycleError())
    {
        std::fprintf(stderr, "%s\n", error->getDiagMessage().c_str());
        return 1;
    }
    return 0;
#endif
}
