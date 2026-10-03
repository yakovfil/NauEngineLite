// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <string_view>

#include "nau/memory/standard_aliases.h"
#include "nau/scene/nau_object.h"
#include "nau/scene/scene.h"

namespace nau::scene
{
    /**
     *   An interface for logically combining a group of scenes.
     */
    struct NAU_ABSTRACT_TYPE IWorld : NauObject
    {
        NAU_INTERFACE(nau::scene::IWorld, NauObject)

        using WeakRef = ObjectWeakRef<IWorld>;

        /**
         * @brief Retrieves the name of the world.
         *
         * @return Name of the scene.
         */
        virtual std::string_view getName() const = 0;

        /**
         * @brief Assigns the name to the world.
         *
         * @param [in] name Name to assign.
         */
        virtual void setName(std::string_view name) = 0;

        /**
         * @brief Retrieves all scenes associated with the world.
         * 
         * @return A collections of scenes associated with the world.
         */
        virtual Vector<IScene::WeakRef> getScenes() const = 0;

        /**
         * @brief Asynchronously attaches the scene to the world.
         * 
         * @param [in] scene    Scene to add.
         * @return              Task object providing the operation status as well as access to the added scene.
         * 
         * @note Calling this renders input scene pointer empty.
         */
        virtual async::Task<IScene::WeakRef> addScene(IScene::Ptr&& scene) = 0;

        /**
         * @brief Detaches the scene from the world and destroys all the contained objects.
         *
         * @param [in] sceneRef Scene to detach.
         * @return              Task object providing the operation status.
         * 
         * See also: removeSceneNoWait.
         */
        virtual void removeScene(IScene::WeakRef sceneRef) = 0;

        /**
         * @brief 
         */
        virtual void setSimulationPause(bool pause) = 0;

        virtual bool isSimulationPaused() const = 0;
    };
}  // namespace nau::scene
