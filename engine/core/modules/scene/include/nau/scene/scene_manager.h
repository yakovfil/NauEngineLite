// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <string>

#include "nau/async/task.h"
#include "nau/memory/standard_aliases.h"
#include "nau/rtti/rtti_object.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_object.h"
#include "nau/scene/scene_query.h"
#include "nau/scene/world.h"

namespace nau::scene
{
    /**
     * @brief Provides interface for managing scenes and worlds.
     */
    struct NAU_ABSTRACT_TYPE ISceneManager
    {
        NAU_TYPEID(nau::scene::ISceneManager)

        /**
         * @brief Destructor.
         */
        virtual ~ISceneManager() = default;

        /**
         * @brief Retrieves the default game world.
         *
         * The default world always exists: it is created automatically and can not be removed.
         */
        virtual IWorld& getDefaultWorld() const = 0;

        /**
         * @brief Retrives all worlds including the default one.
         */
        virtual Vector<IWorld::WeakRef> getWorlds() const = 0;

        virtual IWorld::WeakRef findWorld(Uid worldUid) const = 0;

        /**
         * Creates a new world.
         */
        virtual IWorld::WeakRef createWorld() = 0;

        /**
         * @brief Destroys the world object and deactivates all scenes associated with it. The function will wait until operation is complete.
         *
         * @param [in]  A pointer to the world object to destroy.
         *
         * @note    The function may be called multiple times over the same world.
         *          The second and the following calls will just wait until the operation is complete.
         */
        virtual void destroyWorld(IWorld::WeakRef world) = 0;

        /**
         * @brief Retrieves all active scenes associated with the default world.
         *
         * @return A vector of all active scenes of the default world.
         *
         * See also: IWorld::getScenes and getDefaultWorld.
         */
        virtual Vector<IScene::WeakRef> getActiveScenes() const = 0;

        /**
         * @brief Makes the scene active withing the context of the default world.
         *
         * @param [in] scene    Scene to activate.
         * @return              Task object providing the operation status as well as access to the resulted scene.
         *
         * @note Calling this renders input scene pointer empty.
         */
        virtual async::Task<IScene::WeakRef> activateScene(IScene::Ptr&& scene) = 0;

        /**
         * @brief Detaches the scene from the associated world and destroys all the contained objects. This function waits until the operation is complete.
         *
         * @param [in] sceneRef Scene to deactivate.
         *
         * @note    The function may be called multiple times over the same scene.
         *          The second and the following calls will just wait until the operation is complete.
         */
        virtual void deactivateScene(IScene::WeakRef sceneRef) = 0;

        /**
         */
        virtual ObjectWeakRef<> querySingleObject(const SceneQuery& query) = 0;
    };

    /**
     */
    NAU_CORESCENE_EXPORT SceneQuery createSingleObjectQuery(ObjectWeakRef<> object);

    /**
     */
    NAU_CORESCENE_EXPORT async::Task<IScene::Ptr> openScene(const std::string& path);

}  // namespace nau::scene
