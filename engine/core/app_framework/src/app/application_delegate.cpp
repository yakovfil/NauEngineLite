// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include "nau/app/application_delegate.h"

#include "application_host.h"
#include "nau/app/application.h"
#include "nau/app/application_services.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/app/platform_window.h"
#include "nau/app/window_manager.h"
#include "nau/module/module_manager.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/service/service_provider.h"

namespace nau
{
    namespace
    {
        class DelegateLoop final : public IGamePreUpdate,
                                   public IGamePostUpdate,
                                   public IApplicationHostStartup,
                                   public IRttiObject
        {
            NAU_RTTI_CLASS(DelegateLoop, IGamePreUpdate, IGamePostUpdate, IApplicationHostStartup, IRttiObject)
        public:
            explicit DelegateLoop(ApplicationDelegate& delegate) :
                m_delegate(delegate)
            {
            }

            async::Task<> startupApplicationHost() override
            {
                if (getApplication().isClosing())
                    co_return;
                if (auto* windows = getServiceProvider().find<IWindowManager>())
                    windows->getActiveWindow().setVisible(true);
                m_delegate.onApplicationInitialized();
                if (getApplication().isClosing())
                    co_return;
                co_await m_delegate.startupApplication();
                m_ready = true;
            }
            void gamePreUpdate(std::chrono::milliseconds dt) override
            {
                if (m_ready && !getApplication().isClosing())
                    m_delegate.onApplicationPreUpdate(dt);
            }
            void gamePostUpdate(std::chrono::milliseconds dt) override
            {
                if (m_ready && !getApplication().isClosing())
                    m_delegate.onApplicationPostUpdate(dt);
            }

        private:
            ApplicationDelegate& m_delegate;
            bool m_ready = false;
        };
    }  // namespace

    void ApplicationDelegate::onApplicationPreUpdate(std::chrono::milliseconds)
    {
    }
    void ApplicationDelegate::onApplicationPostUpdate(std::chrono::milliseconds dt)
    {
        onApplicationStep(dt);
    }
    void ApplicationDelegate::onApplicationStep(std::chrono::milliseconds)
    {
    }

    Result<> ApplicationDelegate::initializeApplication()
    {
        const std::string modules = getModulesListString();
        if (!modules.empty())
        {
            NauCheckResult(loadModulesList(modules));
        }
        getServiceProvider().addService(createPlatformWindowService());
        getServiceProvider().addService(std::make_unique<DelegateLoop>(*this));
        return initializeServices();
    }
}  // namespace nau
