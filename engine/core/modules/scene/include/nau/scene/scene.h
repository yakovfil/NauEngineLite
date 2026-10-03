// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <string_view>

#include "nau/scene/nau_object.h"
#include "nau/scene/scene_object.h"


namespace nau::scene
{
    struct IWorld;
    /**
     * @brief Encapsulates a scene and all objects associated with it.
     */
    struct NAU_ABSTRACT_TYPE IScene : NauObject
    {
        NAU_INTERFACE(nau::scene::IScene, NauObject)

        using Ptr = ObjectUniquePtr<IScene>;
        using WeakRef = ObjectWeakRef<IScene>;

        /**
         * @brief Retrieves the name of the scene.
         * 
         * @return Name of the scene.
         */
        virtual std::string_view getName() const = 0;

        /**
         * @brief Assigns the name to the scene.
         * 
         * @param [in] name Name to assign.
         */
        virtual void setName(std::string_view name) = 0;

        /**
         * @brief Retrieves the world which the scene is associated with.
         * 
         * @return A pointer to the retrieved world.
         */
        virtual IWorld* getWorld() const = 0;

        /**
         * @brief Retrives the sceneroot object.
         * 
         * @return A reference to the scene root object.
         */
        virtual SceneObject& getRoot() const = 0;
    };

}  // namespace nau::scene
