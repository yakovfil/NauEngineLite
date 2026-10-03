// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <span>

#include "nau/rtti/type_info.h"

namespace nau::scene
{
    class Component;
    class SceneObject;

    /**
     */
    struct NAU_ABSTRACT_TYPE ISceneListener
    {
        virtual ~ISceneListener() = default;

        /**
            @brief a scene update starts. 
         */
        virtual void onSceneBegin() = 0;

        /**
            @brief the scene update has completed.
         */
        virtual void onSceneEnd() = 0;

        /**
            @brief after completing the activation of newly added objects
            This notification WILL NOT be called for child objects or components.
         */
        virtual void onAfterActivatingObjects(std::span<const SceneObject*> objects) = 0;

        /**
            @brief just before removing objects from the active scene
            This notification WILL NOT be called for child objects (there is also no separate notification for components of objects being deleted)
         */
        virtual void onBeforeDeletingObjects(std::span<const SceneObject*> objects) = 0;

        /**
            @brief after completing the activation of newly added components
            This notification only called when component added to the existing object 
         */
        virtual void onAfterActivatingComponents(std::span<const Component*> components) = 0;

        /**
            @brief just before removing components from the active scene
            This notification WILL NOT be called when whole object is removed.
         */
        virtual void onBeforeDeletingComponents(std::span<const Component*> components) = 0;

        /**
            @brief called when components data/public properties are changed.
            This notification is called deferred, even if the component has changed several times during the frame -
            it will be in the collection only once.

         */
        virtual void onComponentsChange(std::span<const Component*> components) = 0;
    };

}  // namespace nau::scene
