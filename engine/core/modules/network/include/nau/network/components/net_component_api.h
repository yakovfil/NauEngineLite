// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// network_component_api.h

#pragma once
#include <string>
#include <string_view>

#include "nau/memory/bytes_buffer.h"
#include "nau/rtti/rtti_object.h"
#include "nau/scene/scene_object.h"

namespace nau
{
    /**
      * @brief Provides an interface for serializing and deserializing a scene object component.
      */
    struct NAU_ABSTRACT_TYPE IComponentNetSync
    {
        NAU_TYPEID(IComponentNetSync)

        /**
         * @brief Retrieves the scene name.
         * 
         * @return Scene name.
         */
        virtual std::string_view getSceneName() = 0;
        /**
         * @brief Retrieves the absolute path to to the parent object in the scene.
         * 
         * @return Absolute path to to the parent object.
         */
        virtual std::string_view getComponentPath() = 0;

        /**
         * @brief Changes whether the component is owned or replicated.
         * 
         * @param [in] isReplicated Indicates whether the component should be owned or replicated.
         */
        virtual void setIsReplicated(bool isReplicated) = 0;

        /**
         * @brief Checks whether the component is owned or replicated.
         * 
         * @return `true` if the component is owned by the local peer, `false` otherwise (i.e. if it is updated from the remote peer).
         */
        virtual bool isReplicated() const = 0;

        /**
         * @brief Serializes the component into a binary buffer.
         * 
         * @param [out] buffer Buffer storing the serialized data.
         */
        virtual void netWrite(BytesBuffer& buffer) = 0;

        /**
         * @brief Deserializes the binary buffer into the scene component.
         * 
         * @param [in] buffer Buffer with serialized data.
         */
        virtual void netRead(const BytesBuffer& buffer) = 0;

        /**
         * @brief Serializes the component into a JSON text buffer.
         * 
         * @param [out] buffer Buffer storing the serialized data.
         */
        virtual void netWrite(std::string& buffer) = 0;

        /**
         * @brief Deserializes the JSON text buffer into the scene component.
         * 
         * @param [in] buffer Buffer with serialized data.
         */
        virtual void netRead(const std::string& buffer) = 0;
    };

    /**
     * @brief Provides an interface for scene and scene objects components tracking.
     */
    struct NAU_ABSTRACT_TYPE IComponentNetScene
    {
        NAU_TYPEID(nau::IComponentNetScene)

        /**
         * @brief Retrieves the peer ID.
         * 
         * @return Peer ID.
         */
        virtual std::string_view getPeerId() = 0;

        /**
         * @brief Retrieves the scene name.
         * 
         * @return Scene name.
         */
        virtual std::string_view getSceneName() = 0;

        /**
         * @brief Retrieves the network component, possibly creating it.
         * 
         * @return Pointer to the IComponentNetSync instance which is valid until scene deactivation.
         * 
         */
        virtual IComponentNetSync* getOrCreateComponent(std::string_view path, std::string_view type) = 0;
    };
}  // namespace nau
