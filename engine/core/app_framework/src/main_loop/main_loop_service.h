// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <memory>
#include <vector>

#include "app/platform_window_service.h"
#include "concurrent_execution_container.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/rtti/rtti_impl.h"
#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
    #include "nau/scene/internal/scene_manager_internal.h"
#endif
#include "nau/service/service.h"

namespace nau
{
    class MainLoopService final : public IServiceInitialization,
                                  public IServiceShutdown
    {
        NAU_RTTI_CLASS(nau::MainLoopService, IServiceInitialization, IServiceShutdown)

    public:
        void doGameStep(float dt);

        void setApplicationStartupTask(async::Task<> task);

        bool isApplicationStartupReady() const;

        async::Task<> shutdownMainLoop();

        void pollShutdown();

    private:
        async::Task<> preInitGameSystem(IClassDescriptor& systemClass);

        async::Task<> preInitService() override;
        async::Task<> initService() override;
        std::vector<const rtti::TypeInfo*> getServiceDependencies() const override
        {
            return {&rtti::getTypeInfo<PlatformWindowService>()};
        }
        async::Task<> shutdownService() override;

        std::vector<IGamePreUpdate*> m_preUpdate;
        async::Task<> m_applicationStartupTask;
        std::vector<IGamePostUpdate*> m_postUpdate;
        std::vector<IGameSceneUpdate*> m_sceneUpdate;
        std::vector<std::unique_ptr<ConcurrentExecutionContainer> > m_concurrentContainers;

#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
        scene::ISceneManagerInternal* m_sceneManager = nullptr;
#endif
    };

}  // namespace nau
