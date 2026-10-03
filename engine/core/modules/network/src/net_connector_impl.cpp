// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// net_snapshots_impl.cpp

#include "net_connector_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "nau/network/napi/networking_factory.h"
#include "nau/service/service_provider.h"

namespace nau
{
    async::Task<> NetConnectorImpl::preInitService()
    {
        return async::Task<>::makeResolved();
    }

    async::Task<> NetConnectorImpl::initService()
    {
        return async::Task<>::makeResolved();
    }

    void NetConnectorImpl::init()
    {
        if (!m_networking)
        {
            auto service = nau::getServiceProvider().find<NetworkingFactory>();
            m_networking = service->create("ASIO");
            m_networking->init();
        }
    }

    void NetConnectorImpl::init(std::unique_ptr<INetworking>& net)
    {
        m_networking = std::move(net);
    }

    void NetConnectorImpl::listen(const std::string& localPeerId, const std::string& remotePeerId, const std::string& url)
    {
        auto listener = m_networking->createListener();
        m_listeners.push_back({
            listener,
            {localPeerId, remotePeerId, url}
        });
        listener->listen(
            url,
            [this, localPeerId, remotePeerId](std::shared_ptr<nau::INetworkingTransport> incomingTransport) -> void
        {
            auto connection = std::make_shared<Connection>(localPeerId, remotePeerId);
            connection->m_transport = incomingTransport;
            connection->m_state = Connection::State::accepted;
            m_connections.push_back(connection);
        },
            []() -> void
        {
        });
    }

    void NetConnectorImpl::getListeners(std::vector<ConnectionData>& listeners)
    {
        listeners.clear();
        listeners.reserve(m_listeners.size());
        for (auto& listener : m_listeners)
        {
            listeners.push_back(listener.second);
        }
    }

    void NetConnectorImpl::connect(const std::string& localPeerId, const std::string& remotePeerId, const std::string& url)
    {
        auto connector = m_networking->createConnector();
        m_connectors.push_back({
            connector,
            {localPeerId, remotePeerId, url}
        });
        connector->connect(
            url,
            [this, localPeerId, remotePeerId, url](std::shared_ptr<nau::INetworkingTransport> outgoingTransport) -> void
        {
            auto connection = std::make_shared<Connection>(localPeerId, url);
            connection->m_remotePeerId = remotePeerId;
            connection->m_transport = outgoingTransport;
            connection->m_state = Connection::State::connected;
            m_connections.push_back(connection);
        },
            []() -> void
        {
        });
    }

    void NetConnectorImpl::getConnectors(std::vector<ConnectionData>& connectors)
    {
        connectors.clear();
        connectors.reserve(m_connectors.size());
        for (auto& connector : m_connectors)
        {
            connectors.push_back(connector.second);
        }
    }

    void NetConnectorImpl::getConnections(const std::string& peerId, std::vector<std::string>& peers)
    {
        peers.clear();
        for (auto& connection : m_connections)
        {
            if (connection->m_localPeerId == peerId)
            {
                if (!connection->m_remotePeerId.empty())
                {
                    peers.push_back(connection->m_remotePeerId);
                }
            }
        }
    }

    void NetConnectorImpl::getConnections(std::vector<std::weak_ptr<IConnection>>& connections)
    {
        connections.clear();
        for (auto& connection : m_connections)
        {
            connections.push_back(connection);
        }
    }

    void NetConnectorImpl::writeFrame(const std::string& peerId, const std::string& frame)
    {
        for (auto& connection : m_connections)
        {
            if (connection->m_localPeerId == peerId)
            {
                connection->writeFrame(frame);
            }
        }
    }

    bool NetConnectorImpl::readFrame(const std::string& peerId, const std::string& fromPeerId, std::string& frame)
    {
        frame.clear();
        for (auto& connection : m_connections)
        {
            if (connection->m_localPeerId == peerId && connection->m_remotePeerId == fromPeerId)
            {
                if (!connection->m_frameBuffer.empty())
                {
                    frame = connection->m_frameBuffer;
                    return true;
                }
            }
        }
        return false;
    }

    void NetConnectorImpl::update()
    {
        if (m_networking)
        {
            m_networking->update();
        }
        for (auto& connection : m_connections)
        {
            connection->update();
        }
    }

    void NetConnectorImpl::Connection::update()
    {
        if (m_transport)
        {
            if (m_transport->isConnected())
            {
                std::vector<NetworkingMessage> messages;
                m_transport->read(messages);
                // TODO - move message handling to ASIO transport implementation
                for (auto& message : messages)
                {
                    m_recBuffer.append(asStringView(message.buffer).data());
                }
                processMessages();
                if (m_remotePeerId.empty())
                {
                    requestRemoteId();
                }
            }
        }
    }

    void NetConnectorImpl::Connection::processMessages()
    {
        while (true)
        {
            size_t start = m_recBuffer.find('\n');
            if (start == std::string::npos)
            {
                return;
            }
            size_t end = m_recBuffer.find('\r', start);
            if (end == std::string::npos)
            {
                return;
            }
            std::string message = m_recBuffer.substr(start, end - start + 1);
            m_recBuffer.erase(start, end - start + 1);
            processMessage(message);
        }
    }

    void NetConnectorImpl::Connection::processMessage(const std::string& message)
    {
        if (message == "\n|req_id\r")
        {
            sendId();
            return;
        }
        if (message.find("\n|id", 0) == 0)
        {
            m_remotePeerId = message.substr(4, message.size() - 5);
            return;
        }
        if (message.find("{") == 1)  // frame data assumed
        {
            m_frameBuffer = message;
        }
    }

    void NetConnectorImpl::Connection::writeFrame(const std::string& frame)
    {
        NetworkingMessage message("\n" + frame + "\r");
        m_transport->write(message);
    }

    void NetConnectorImpl::Connection::requestRemoteId()
    {
        NetworkingMessage message("\n|req_id\r");
        m_transport->write(message);
        if (m_verbose)
        {
            // NAU_LOG_DEBUG("requestRemoteId()");
        }
    }

    void NetConnectorImpl::Connection::sendId()
    {
        NetworkingMessage message("\n|id" + m_localPeerId + "\r");
        m_transport->write(message);
        if (m_verbose)
        {
            // NAU_LOG_DEBUG("sendId()");
        }
    }

}  // namespace nau