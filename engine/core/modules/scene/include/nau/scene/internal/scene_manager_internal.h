// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/async/task_base.h"
#include "nau/rtti/type_info.h"
#include "nau/scene/components/component.h"

namespace nau::scene
{
    struct ISceneListener;

    /**
     */
    // TODO: ability to exclude SceneListener mechanism from build
    struct [[nodiscard]] NAU_CORESCENE_EXPORT SceneListenerRegistration
    {
        SceneListenerRegistration() = default;
        SceneListenerRegistration(SceneListenerRegistration&&);
        SceneListenerRegistration(const SceneListenerRegistration&) = delete;
        ~SceneListenerRegistration();
        SceneListenerRegistration& operator=(const SceneListenerRegistration&) = delete;
        SceneListenerRegistration& operator=(SceneListenerRegistration&&);

        explicit operator bool() const;
        void reset();
        ISceneListener* getListener() const;

    private:
        SceneListenerRegistration(void* handle);

        void* m_handle = nullptr;

        friend class SceneManagerImpl;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE ISceneManagerInternal
    {
        NAU_TYPEID(nau::scene::ISceneManagerInternal)

        virtual void update(float dt) = 0;

        virtual Component* findComponent(Uid componentId) = 0;

        virtual async::Task<> shutdown() = 0;

        // Resume outstanding scene work without beginning a new component update.
        virtual void pollShutdown() = 0;

        virtual SceneListenerRegistration addSceneListener(ISceneListener&) = 0;
    };

}  // namespace nau::scene
