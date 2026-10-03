// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <span>

#include "nau/async/task.h"
#include "nau/rtti/rtti_object.h"
#include "nau/scene/components/component.h"

namespace nau::scene
{
    /**
     * @brief Provides interface for activating & deactivating components.
     */
    struct NAU_ABSTRACT_TYPE IComponentsActivator
    {
        NAU_TYPEID(nau::scene::IComponentsActivator)

        virtual Result<> activateComponents(Uid worldUid, std::span<Component*> components)
        {
            return ResultSuccess;
        }

        virtual void deactivateComponents(Uid worldUid, std::span<Component*> components)
        {
        }
    };

    /**
        @brief Deactivated component data for async handlers.
        When component (or owner object) is going to be deactivated it 
     */
    struct DeactivatedComponentData
    {
        const Component* component;
        Uid componentUid;
        Uid parentObjectUid;
        Uid sceneUid;
        Uid worldUid;
    };


    /**
     * @brief Provides interface for asynchronously activating & deactivating components.
     */
    struct NAU_ABSTRACT_TYPE IComponentsAsyncActivator
    {
        NAU_TYPEID(nau::scene::IComponentsAsyncActivator)

        

        /**
         * @brief Asynchronously activates components.
         *
         * @param [in] components   A collection of components to activate.
         * @param [in] barrier
         * @return                  Task object providing status of the operation.
         */
        virtual async::Task<> activateComponentsAsync(Uid worldUid, std::span<const Component*> components, async::Task<> barrier)
        {
            return async::makeResolvedTask();
        }

        /**
         * @brief Asynchronously deactivates components.
         *
         * @param [in] components   A collection of components to deactivate.
         * @return                  Task object providing status of the operation.
         */
        virtual async::Task<> deactivateComponentsAsync(Uid worldUid, std::span<const DeactivatedComponentData> components)
        {
            return async::makeResolvedTask();
        }
    };

    /**
     */
    struct NAU_ABSTRACT_TYPE ISceneProcessor : virtual IRttiObject
    {
        NAU_INTERFACE(nau::scene::ISceneProcessor)

        virtual void syncSceneState() = 0;
    };

    /**
     */
    template <std::derived_from<Component> T, std::derived_from<Component>... U>
    inline bool hasAcceptableComponent(std::span<const Component*> components)
    {
        for (const Component* const comp : components)
        {
            if (comp->is<T>() || (comp->is<U>() || ...))
            {
                return true;
            }
        }

        return false;
    }

    /**
     */
    template <std::derived_from<Component> T, std::derived_from<Component>... U>
    inline bool hasAcceptableComponent(std::span<Component*> components)
    {
        for (const Component* const comp : components)
        {
            if (comp->is<T>() || (comp->is<U>() || ...))
            {
                return true;
            }
        }

        return false;
    }    
}  // namespace nau::scene
