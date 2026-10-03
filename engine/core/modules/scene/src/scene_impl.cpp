// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "./scene_impl.h"

#include <string_view>

#include "./world_impl.h"
#include "nau/scene/scene_factory.h"
#include "nau/service/service_provider.h"

namespace nau::scene
{
    SceneImpl::SceneImpl() :
        m_sceneRoot(getServiceProvider().get<ISceneFactory>().createSceneObject())
    {
        m_sceneRoot->setScene(this);
    }

    std::string_view SceneImpl::getName() const
    {
        return m_name;
    }

    void SceneImpl::setName(std::string_view name)
    {
        m_name = name;
    }

    IWorld* SceneImpl::getWorld() const
    {
        return m_world ? m_world.get() : nullptr;
    }

    void SceneImpl::setWorld(WorldImpl& world)
    {
        // Currently scene's world can be set only once
        NAU_ASSERT(!m_world, "Can not change or reset world");
        m_world = world;
    }

    SceneObject& SceneImpl::getRoot() const
    {
        NAU_ASSERT(m_sceneRoot);
        return *m_sceneRoot;
    }

}  // namespace nau
