// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <chrono>
#include <optional>

#include "nau/async/task_base.h"
#include "nau/meta/attribute.h"
#include "nau/rtti/type_info.h"
#include "nau/utils/enum/enum_reflection.h"

namespace nau
{
    /**
     */
    NAU_DEFINE_ENUM_(
        ExecutionMode,
        Sequential,
        Concurrent)

    /**
     */
    NAU_DEFINE_ENUM_(
        SceneAccess,
        ReadOnly,
        Modify)

    /**
     */
    NAU_DEFINE_ATTRIBUTE_(PreferredExecutionMode);

    /**
     */
    NAU_DEFINE_ATTRIBUTE_(SceneAccessMode)

    /**
     */
    NAU_DEFINE_ATTRIBUTE_(GameSystemName)

    /** Samples platform input once on the application owner thread, before game updates. */
    struct NAU_ABSTRACT_TYPE IGameInputUpdate
    {
        NAU_TYPEID(nau::IGameInputUpdate)

        virtual ~IGameInputUpdate() = default;
        virtual void gameInputUpdate(std::chrono::milliseconds dt) = 0;
    };

    struct NAU_ABSTRACT_TYPE IGamePreUpdate
    {
        NAU_TYPEID(nau::IGamePreUpdate)

        virtual ~IGamePreUpdate() = default;

        virtual void gamePreUpdate(std::chrono::milliseconds dt) = 0;
    };

    struct NAU_ABSTRACT_TYPE IGamePostUpdate
    {
        NAU_TYPEID(nau::IGamePostUpdate)

        virtual ~IGamePostUpdate() = default;

        virtual void gamePostUpdate(std::chrono::milliseconds dt) = 0;
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE IGameSceneUpdate
    {
        NAU_TYPEID(nau::IGameSceneUpdate)

        /**
         */
        virtual async::Task<bool> update(std::chrono::milliseconds dt) = 0;

        /**
         */
        virtual std::optional<std::chrono::milliseconds> getFixedUpdateTimeStep() = 0;

        /**
         */
        virtual void syncSceneState() = 0;
    };

}  // namespace nau
