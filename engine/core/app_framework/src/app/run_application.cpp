// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/app/run_application.h"

#include <memory>
#include <string>

#include "main_loop/main_loop_service.h"
#include "nau/app/application.h"
#include "nau/app/application_services.h"
#include "nau/app/core_window_manager.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/app/platform_window.h"
#include "nau/input.h"
#include "nau/module/module_manager.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    namespace
    {
        class DelegateLoop final : public IGamePreUpdate,
                                   public IGamePostUpdate,
                                   public IRttiObject
        {
            NAU_RTTI_CLASS(nau::DelegateLoop, IGamePreUpdate, IGamePostUpdate, IRttiObject)

        public:
            DelegateLoop(ApplicationDelegate& appDelegate) :
                m_appDelegate(appDelegate)
            {
            }

            void gamePreUpdate(std::chrono::milliseconds dt) override
            {
            }

            void gamePostUpdate(std::chrono::milliseconds dt) override
            {
                if (!getServiceProvider().get<MainLoopService>().isApplicationStartupReady())
                {
                    return;
                }

                if (getApplication().isClosing())
                {
                    return;
                }

                m_appDelegate.onApplicationStep(dt);
            }

            void startupAppDelegate()
            {
                getServiceProvider().get<MainLoopService>().setApplicationStartupTask(m_appDelegate.startupApplication());
            }

        private:
            ApplicationDelegate& m_appDelegate;
        };
    }  // namespace

    void ApplicationDelegate::onApplicationStep([[maybe_unused]] std::chrono::milliseconds dt)
    {
    }

    Result<> ApplicationDelegate::initializeApplication()
    {
#if !defined(NAU_STATIC_RUNTIME)
        const std::string moduleList = getModulesListString();
        if (!moduleList.empty())
        {
            NauCheckResult(loadModulesList(moduleList));
        }
#endif
        getServiceProvider().addService(createPlatformWindowService());

        getServiceProvider().addService(std::make_unique<DelegateLoop>(*this));

        return initializeServices();
    }

    int runApplication(ApplicationDelegate::Ptr appDelegate)
    {
        NAU_FATAL(appDelegate);

        auto app = createApplication(*appDelegate);
        if (!app)
        {
            return -1;
        }

        app->startupOnCurrentThread();
        getServiceProvider().get<IWindowManager>().getActiveWindow().setVisible(true);

        appDelegate->onApplicationInitialized();
        getServiceProvider().get<DelegateLoop>().startupAppDelegate();

        while (app->step())
        {
            // TODO Tracy            NAU_CPU_SCOPED;
            // TODO Tracy            NAU_PROFILING_FRAME_END;
        }

        return 0;
    }
}  // namespace nau
