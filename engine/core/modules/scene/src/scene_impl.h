// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>
#include <string_view>

#include "nau/scene/scene.h"
#include "nau/scene/scene_object.h"

namespace nau::scene
{
    class SceneImpl final : public IScene
    {
        NAU_OBJECT(nau::scene::SceneImpl, IScene)

    public:
        SceneImpl();

        std::string_view getName() const override;

        void setName(std::string_view name) override;

        IWorld* getWorld() const override;

        SceneObject& getRoot() const override;

    private:
        void setWorld(class WorldImpl&);

        std::string m_name;
        const SceneObject::Ptr m_sceneRoot;
        mutable ObjectWeakRef<WorldImpl> m_world;

        friend class SceneManagerImpl;
    };

}  // namespace nau
