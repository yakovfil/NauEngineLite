// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// net_snapshots.h

#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nau/network/napi/networking.h"
#include "nau/network/netsync/net_connector.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"
#include "nau/rtti/type_info.h"
#include "nau/service/service.h"

namespace nau
{
    class NetConnectorImpl final : public IServiceInitialization,
                                   public INetConnector
    {
        NAU_RTTI_CLASS(nau::NetConnectorImpl, IServiceInitialization, INetConnector)
    public:
        async::Task<> preInitService() override;
        async::Task<> initService() override;

        void init() override;
        void init(std::unique_ptr<INetworking>& net);

        void listen(const std::string& localPeerId, const std::string& remotePeerId, const std::string& url) override;
        void getListeners(std::vector<ConnectionData>& listeners) override;

        void connect(const std::string& localPeerId, const std::string& remotePeerId, const std::string& url) override;
        void getConnectors(std::vector<ConnectionData>& connectors) override;

        void getConnections(const std::string& peerId, std::vector<std::string>& peers) override;
        void getConnections(std::vector<std::weak_ptr<IConnection>>& connections) override;

        void writeFrame(const std::string& peerId, const std::string& frame) override;
        bool readFrame(const std::string& peerId, const std::string& fromPeerId, std::string& frame) override;

        void update() override;

    private:
        struct Connection : public IConnection
        {
            std::string m_localPeerId;
            std::string m_remotePeerId;
            State m_state = none;
            std::shared_ptr<INetworkingTransport> m_transport;

            // Temporary measure until we implement some message protocol for stream connections, like ASIO TCP
            std::string m_recBuffer;

            std::string m_frameBuffer;

            Connection(const std::string& localPeerId, const std::string& remotePeerId) :
                m_localPeerId(localPeerId),
                m_remotePeerId(remotePeerId)
            {
            }

            virtual State state() override
            {
                return m_state;
            }

            virtual const std::string& localPeerId() override
            {
                return m_localPeerId;
            }

            virtual const std::string& remotePeerId() override
            {
                return m_remotePeerId;
            }

            virtual const std::string& localEndPoint() override
            {
                return m_transport->localEndPoint();
            }

            virtual const std::string& remoteEndPoint() override
            {
                return m_transport->remoteEndPoint();
            }

            void update();
            void processMessages();
            void processMessage(const std::string& message);
            void writeFrame(const std::string& frame);
            void requestRemoteId();
            void sendId();

            // Debug
            bool m_verbose = true;
        };

        std::unique_ptr<INetworking> m_networking;
        std::vector<std::pair<std::shared_ptr<INetworkingListener>, ConnectionData>> m_listeners;
        std::vector<std::pair<std::shared_ptr<INetworkingConnector>, ConnectionData>> m_connectors;
        std::vector<std::shared_ptr<Connection>> m_connections;

        // Debug
        bool m_verbose = true;
    };
}  // namespace nau