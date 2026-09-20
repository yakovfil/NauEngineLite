// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

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

        async::Task<> shutdownMainLoop();

        void pollShutdown();

    private:
        async::Task<> preInitGameSystem(IClassDescriptor& systemClass);

        async::Task<> preInitService() override;
        async::Task<> initService() override;
        eastl::vector<const rtti::TypeInfo*> getServiceDependencies() const override
        {
            return {&rtti::getTypeInfo<PlatformWindowService>()};
        }
        async::Task<> shutdownService() override;

        eastl::vector<IGamePreUpdate*> m_preUpdate;
        eastl::vector<IGamePostUpdate*> m_postUpdate;
        eastl::vector<IGameSceneUpdate*> m_sceneUpdate;
        eastl::vector<eastl::unique_ptr<ConcurrentExecutionContainer> > m_concurrentContainers;

#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
        scene::ISceneManagerInternal* m_sceneManager = nullptr;
#endif
    };

}  // namespace nau
