// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <string>
#include <string_view>
#include <vector>

#include "nau/diag/logging.h"
#include "nau/network/components/net_component_api.h"
#include "nau/network/components/net_scene_component.h"
#include "nau/network/netsync/net_snapshots.h"
#include "nau/scene/components/component.h"
#include "nau/scene/components/component_attributes.h"
#include "nau/scene/components/component_life_cycle.h"
#include "nau/scene/components/internal/component_internal_attributes.h"
#include "nau/service/service_provider.h"

namespace nau
{
    /**
     * @brief Provides common funtionality for IComponentNetSync derived components.
     */
    class NetSyncBaseComponent : public nau::scene::Component,
                                 public nau::scene::IComponentEvents,
                                 public nau::scene::IComponentUpdate,
                                 public IComponentNetSync
    {
        NAU_OBJECT(NetSyncBaseComponent, Component, IComponentUpdate, IComponentEvents, IComponentNetSync)

        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_ATTRIBUTES(
            CLASS_ATTRIBUTE(scene::SystemComponentAttrib, true),
            CLASS_ATTRIBUTE(scene::ComponentDisplayNameAttrib, "Net Sync Base"),
            CLASS_ATTRIBUTE(scene::ComponentDescriptionAttrib, "Net Sync Base (description)"))

    protected:
        /**
         * @brief Changes whether the component is replicated from the remote peer or owned by the local peer.
         * 
         * @param [in] isReplicated Indicates whether the component is replicated from the remote peer or owned by the local peer.
         */
        void setIsReplicated(bool isReplicated) override
        {
            m_isReplicated = isReplicated;
        }

        /**
         * @brief Checks whether the component is replicated from the remote peer or owned by the local peer.
         * 
         * @return `false` if the component is owned by the local peer, `true` if it is replicated from the remote peer.
         */
        bool isReplicated() const override
        {
            return m_isReplicated;
        }

        /*
         * @brief Retrieves the scene name.
         * 
         * @return Scene name.
         */
        std::string_view getSceneName() override
        {
            return m_scene->getSceneName();
        }

        /*
         * @brief Retrieves the component full path.
         * 
         * @return Full path to the component.
         */
        std::string_view getComponentPath() override
        {
            return m_path;
        }

        /**
        * @brief Noop, see IComponentNetSync::netWrite.
        *
        * @param [out] buffer
        */
        virtual void netWrite(BytesBuffer& buffer) override
        {
        }

        /**
        * @brief Noop, see IComponentNetSync::netRead.
        *
        * @param [out] buffer
        */
        virtual void netRead(const BytesBuffer& buffer) override
        {
        }

        /**
         * @brief Noop, see IComponentNetSync::netWrite.
         * 
         * @param [out] buffer
         */
        virtual void netWrite(std::string& buffer) override
        {
        }

        /**
         * @brief Noop, see IComponentNetSync::netRead
         *
         * @param [in] buffer 
         */
        virtual void netRead(const std::string& buffer) override
        {
        }

        /*
         * @brief Updates component state in snapshot manager.
         * 
         * @param [in] dt Time in seconds elapsed since the last update.
         */
        void updateComponent(float dt) override
        {
            if (!m_isReplicated && m_snapshots != nullptr)
            {
                m_snapshots->onComponentWrite(this);
            }
        }

        /*
         * @brief Adds the component to the snapshot manager.
         */
        void onComponentActivated() override
        {
            if (!m_activated)
            {
                m_activated = true;
                if (m_snapshots = getServiceProvider().find<INetSnapshots>())
                {
                    m_path = getObjectPath("/");
                    m_scene = getRootObject()->findFirstComponent<NetSceneComponent>();
                    m_snapshots->onComponentActivated(this);
                }
                else
                {
                    NAU_LOG_ERROR("No NetSnapshots service found");
                }
            }
        }

        /*
         * @brief Finds the root scene object.
         * 
         * @return A pointer to the root SceneObject that is valid until the scene is deactivated
         */
        scene::SceneObject* getRootObject()
        {
            auto* parent = getParentObject().getParentObject();
            while (parent != nullptr)
            {
                auto* pp = parent->getParentObject();
                if (pp == nullptr)
                {
                    break;
                }
                parent = pp;
            }
            return parent;
        }

        /*
         * @brief Calculates full component path.

         * @param [in] delimiter    Delimiter to separate subdirectories.
         * @return                  Full object path relative to root.
         */
        std::string getObjectPath(const std::string& delimiter)
        {
            scene::SceneObject* obj = &getParentObject();
            std::vector<const scene::SceneObject*> objs;
            objs.reserve(16);
            objs.push_back(obj);
            auto* parent = obj->getParentObject();
            while (parent != nullptr)
            {
                objs.push_back(parent);
                parent = parent->getParentObject();
            }
            std::string path;
            while (objs.size() > 0)
            {
                path += delimiter;
                path += objs.back()->getName();
                objs.pop_back();
            }
            return path;
        }

        bool m_isReplicated = false;
        bool m_activated = false;
        INetSnapshots* m_snapshots = nullptr;
        IComponentNetScene* m_scene = nullptr;
        std::string m_path;
    };
}  // namespace nau
