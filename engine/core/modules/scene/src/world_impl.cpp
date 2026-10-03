// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./world_impl.h"

#include <string_view>

#include "scene_management/scene_manager_impl.h"

namespace nau::scene
{
    WorldImpl::WorldImpl()
    {
        setUid(Uid::generate());
    }

    SceneManagerImpl& WorldImpl::getSceneManager() const
    {
        return getServiceProvider().get<SceneManagerImpl>();
    }

    std::string_view WorldImpl::getName() const
    {
        return m_name;
    }

    void WorldImpl::setName(std::string_view name)
    {
        m_name = name;
    }

    Vector<IScene::WeakRef> WorldImpl::getScenes() const
    {
        return getSceneManager().getActiveScenes(this);
    }

    async::Task<IScene::WeakRef> WorldImpl::addScene(IScene::Ptr&& scene)
    {
        return getSceneManager().activateScene(*this, std::move(scene));
    }

    void WorldImpl::removeScene(IScene::WeakRef sceneRef)
    {
        getSceneManager().deactivateScene(sceneRef);
    }

    void WorldImpl::setSimulationPause(bool pause)
    {
        m_isPaused = pause;
    }

    bool WorldImpl::isSimulationPaused() const
    {
        return m_isPaused;
    }
}  // namespace nau::scene
