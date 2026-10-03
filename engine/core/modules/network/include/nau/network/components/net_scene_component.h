// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>
#include <string_view>

#include "nau/async/task_base.h"
#include "nau/diag/logging.h"
#include "nau/network/components/net_component_api.h"
#include "nau/network/netsync/net_snapshots.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/scene/scene.h"
#include "nau/scene/scene_manager.h"
#include "nau/serialization/json_utils.h"
#include "nau/service/service_provider.h"
#include "nau/string/string_utils.h"

namespace nau
{
    /**
     * @brief 
     * Track scene activation/deactivation and manages IComponentNetSync derived components
     */
    class NetSceneComponent final : public nau::scene::Component,
                                    public nau::scene::IComponentEvents,
                                    public nau::scene::IComponentUpdate,
                                    public IComponentNetScene
    {
        NAU_OBJECT(NetSceneComponent, Component, IComponentEvents, IComponentUpdate, IComponentNetScene)

        NAU_DECLARE_DYNAMIC_OBJECT
        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Net Scene"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Net Scene (description)"))

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_peerId, "PeerId"),
            CLASS_NAMED_FIELD(m_sceneName, "SceneName"))

    public:
        NetSceneComponent()
        {
            NAU_LOG("ComponentNetworkScene");
        }

        ~NetSceneComponent()
        {
            NAU_LOG("~ComponentNetworkScene");
        }

        /**
         * @brief Scene name setter
         */
        void setSceneName(std::string_view name)
        {
            m_sceneName = name;
        }

        /**
         * @brief Peer Id setter
         */
        void setPeerId(std::string_view peerId)
        {
            m_peerId = peerId;
        }

        /**
         * @brief Peer Id getter
         */
        virtual std::string_view getPeerId() override
        {
            return m_peerId;
        }

        /**
         * @brief Scene name getter
         */
        virtual std::string_view getSceneName() override
        {
            return m_sceneName;
        }

        /**
         * @brief Get existing or create new component by specific path and type
         * @param path - full path to component
         * @param type - RTTI TypeInfo for component
         * @return Pointer to IComponentNetSync, valid until scene deactivation
         */
        IComponentNetSync* getOrCreateComponent(std::string_view path, std::string_view type) override
        {
            auto* sceneObject = findObjectByPath(&getParentObject(), path, "/");
            if (sceneObject != nullptr)
            {
                auto components = sceneObject->getAllComponents();
                for (auto& component : components)
                {
                    auto* netSync = component->as<IComponentNetSync*>();
                    if (netSync != nullptr)
                    {
                        return netSync;
                    }
                }
            }
            NAU_LOG_WARNING("getOrCreateComponent failed");
            return nullptr;
        }

    private:

        void updateComponent(float dt)
        {
        }

        void onComponentCreated() override
        {
        }

        /**
         * @brief Register activated components in snapshots service
         */
        void onComponentActivated() override
        {
            if (!m_activated)
            {
                m_activated = true;
                NAU_LOG("ComponentNetworkScene onComponentActivated");

                if (m_snapshots = getServiceProvider().find<INetSnapshots>())
                {
                    m_snapshots->onSceneActivated(this);
                }
                else
                {
                    NAU_LOG_ERROR("No NetSnapshots service found");
                }
            }
        }

        /**
         * @brief Find child of specefied object by name
         * @return Pointer to SceneObject, valid until scene was changes
         */
        inline scene::SceneObject* findObjectChild(scene::SceneObject* obj, const std::string_view childName)
        {
            auto childs = obj->getDirectChildObjects();
            for (auto* child : childs)
            {
                if (child->getName() == childName)
                {
                    return child;
                }
            }
            return nullptr;
        }

        /**
         * @brief Find child of specefied object by path
         * @return Pointer to SceneObject, valid until scene was changes
         */
        scene::SceneObject* findObjectByPath(scene::SceneObject* root, const std::string_view path, const std::string_view delimiter)
        {
            if (path.empty())
            {
                return nullptr;
            }
            scene::SceneObject* objPtr = root;
            auto splitted = nau::strings::split(path, delimiter);
            for (auto split : splitted)
            {
                if (objPtr == nullptr)
                {
                    break;
                }
                if (objPtr->getName() != split)
                {
                    objPtr = findObjectChild(objPtr, split);
                }
            }
            return objPtr;
        }

        INetSnapshots* m_snapshots = nullptr;
        std::string m_peerId = "PeerId";
        std::string m_sceneName = "SceneName";
        bool m_activated = false;
    };

}  // namespace nau
