// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "main_loop_service.h"

#ifndef NAU_MINIMAL_RUNTIME
    #include "nau/gui/dag_imgui.h"
#endif

namespace nau
{
    namespace
    {
        struct SystemEntry
        {
            IClassDescriptor* classDescriptor = nullptr;
            bool collectDependencies = true;
            eastl::unordered_set<rtti::TypeIndex> dependencies;
        };

        SystemEntry& getSystemEntry(const eastl::vector<IClassDescriptor::Ptr>& allSystems, eastl::list<SystemEntry>& systems, IClassDescriptor& classDescriptor)
        {
            auto entry = eastl::find_if(systems.begin(), systems.end(), [&classDescriptor](const SystemEntry& entry)
            {
                return entry.classDescriptor == &classDescriptor;
            });

            if (entry == systems.end())
            {
                systems.emplace_back(&classDescriptor);
                SystemEntry& newEntry = systems.back();
                NAU_FATAL(newEntry.collectDependencies);
                scope_on_leave
                {
                    newEntry.collectDependencies = false;
                };

                return newEntry;
            }

            NAU_FATAL(entry != systems.end());
            NAU_FATAL(!entry->collectDependencies);

            return *entry;
        }
    }  // namespace

    async::Task<> MainLoopService::preInitGameSystem(IClassDescriptor& systemClass)
    {
        NAU_FATAL(systemClass.getConstructor() != nullptr);

        const meta::IRuntimeAttributeContainer* const attributes = systemClass.getClassAttributes();
        NAU_FATAL(attributes);

        if (systemClass.hasInterface<IGameSceneUpdate>())
        {
            if (attributes->get<PreferredExecutionMode, ExecutionMode>().value_or(ExecutionMode::Sequential) == ExecutionMode::Concurrent)
            {
                auto& concurrentContainer = *m_concurrentContainers.emplace_back(eastl::make_unique<ConcurrentExecutionContainer>(Ptr{&systemClass}));
                co_await concurrentContainer.preInitService();
                co_return;
            }
        }

        IRttiObject* const systemInstance = *systemClass.getConstructor()->invoke(nullptr, {});
        NAU_FATAL(systemInstance);
        getServiceProvider().addService(eastl::unique_ptr<IRttiObject>{systemInstance});

        if (auto* const sceneUpdate = systemInstance->as<IGameSceneUpdate*>())
        {
            m_sceneUpdate.push_back(sceneUpdate);
        }

        if (auto* const preUpdate = systemInstance->as<IGamePreUpdate*>())
        {
            m_preUpdate.push_back(preUpdate);
        }

        if (auto* const postUpdate = systemInstance->as<IGamePostUpdate*>())
        {
            m_postUpdate.push_back(postUpdate);
        }

        if (auto* const preInitService = systemInstance->as<IServiceInitialization*>())
        {
            co_await preInitService->preInitService();
        }
    }

    async::Task<> MainLoopService::preInitService()
    {
        if (auto serviceWithPreUpdate = getServiceProvider().getAll<IGamePreUpdate>(); !serviceWithPreUpdate.empty())
        {
            const auto offset = m_preUpdate.size();
            m_preUpdate.resize(serviceWithPreUpdate.size());
            eastl::copy(serviceWithPreUpdate.begin(), serviceWithPreUpdate.end(), m_preUpdate.begin() + offset);
        }

        if (auto serviceWithPostUpdate = getServiceProvider().getAll<IGamePostUpdate>(); !serviceWithPostUpdate.empty())
        {
            const auto offset = m_postUpdate.size();
            m_postUpdate.resize(serviceWithPostUpdate.size());
            eastl::copy(serviceWithPostUpdate.begin(), serviceWithPostUpdate.end(), m_postUpdate.begin() + offset);
        }

        eastl::vector<IClassDescriptor::Ptr> allSystems = getServiceProvider().findClasses<IGamePreUpdate, IGamePostUpdate, IGameSceneUpdate>();
        eastl::list<SystemEntry> orderedSystems;

        for (const IClassDescriptor::Ptr& systemClass : allSystems)
        {
            [[maybe_unused]] auto& entry = getSystemEntry(allSystems, orderedSystems, *systemClass);
        }

        NAU_FATAL(orderedSystems.size() == allSystems.size());

        for (const SystemEntry& entry : orderedSystems)
        {
            co_await preInitGameSystem(*entry.classDescriptor);
        }
    }

    async::Task<> MainLoopService::initService()
    {
#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
        if (getServiceProvider().has<scene::ISceneManagerInternal>())
        {
            m_sceneManager = &getServiceProvider().get<scene::ISceneManagerInternal>();
        }
#endif

        return async::makeResolvedTask();
    }

    async::Task<> MainLoopService::shutdownService()
    {
        return async::makeResolvedTask();
    }

    async::Task<> MainLoopService::shutdownMainLoop()
    {
#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
        if (!m_sceneManager)
        {
            m_sceneManager = getServiceProvider().find<scene::ISceneManagerInternal>();
        }
        if (m_sceneManager)
        {
            co_await m_sceneManager->shutdown();
        }
#endif
        co_return;
    }

    void MainLoopService::pollShutdown()
    {
#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
        if (m_sceneManager)
        {
            m_sceneManager->pollShutdown();
        }
#endif
    }

    void MainLoopService::doGameStep(float dt)
    {
        using namespace std::chrono;

        const milliseconds msDt{static_cast<milliseconds::rep>(1000.f * dt)};

        for (IGamePreUpdate* const preUpdate : m_preUpdate)
        {
            preUpdate->gamePreUpdate(msDt);
        }

#if !defined(NAU_MINIMAL_RUNTIME) || defined(NAU_SCENE_RUNTIME)
        if (m_sceneManager != nullptr)
        {
            m_sceneManager->update(dt);
        }
#endif

        for (IGamePostUpdate* const postUpdate : m_postUpdate)
        {
            postUpdate->gamePostUpdate(msDt);
        }

#ifndef NAU_MINIMAL_RUNTIME
        if (imgui_get_state() != ImGuiState::OFF)
        {
            imgui_cache_render_data();
            imgui_update();
        }
#endif
    }
}  // namespace nau
