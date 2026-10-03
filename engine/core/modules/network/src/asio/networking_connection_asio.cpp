// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/network/asio/networking_connection_asio.h"

#include <uriparser/Uri.h>

#include <asio.hpp>
#include <memory>
#include <string>
#include <utility>

#include "nau/network/asio/networking_transport_asio.h"
#include "networking_asio_wrapper.h"

using namespace asio;
using namespace asio::ip;

namespace nau
{
    namespace NetworkingASIO
    {
        // TODO - move to common Networking sources
        static bool ParseURI(const std::string& uriStr, std::string& scheme, std::string& host, std::string& port)
        {
            UriUriA uri;
            const char* errorPos;
            // No need to call uriFreeUriMembersA if Failure
            if (uriParseSingleUriA(&uri, uriStr.c_str(), &errorPos) == URI_SUCCESS)
            {
                scheme = std::string(uri.scheme.first, uri.scheme.afterLast);
                host = std::string(uri.hostText.first, uri.hostText.afterLast);
                port = std::string(uri.portText.first, uri.portText.afterLast);
                uriFreeUriMembersA(&uri);
                return true;
            }
            return false;
        }
    }  // namespace NetworkingASIO

    NetworkingListenerASIO::NetworkingListenerASIO(asio::io_context& io_context) :
        m_context(io_context)
    {
    }

    NetworkingListenerASIO::~NetworkingListenerASIO() = default;

    bool NetworkingListenerASIO::listen(const std::string& uriStr, nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback, nau::Functor<void(void)> failCallback)
    {
        bool result = false;
        std::string scheme;
        std::string host;
        std::string port;
        if (NetworkingASIO::ParseURI(uriStr, scheme, host, port))
        {
            if (scheme == "tcp")
            {
                m_context.m_successCallback = std::move(successCallback);
                m_context.m_failCallback = std::move(failCallback);
                if (!m_acceptor)
                {
                    m_acceptor = std::make_shared<ASIO_Acceptor>(m_context.m_io_context, std::stoi(port.c_str()), [this](tcp::socket socket) -> void
                    {
                        auto connection = std::make_shared<ASIO_Connection>(std::move(socket));
                        auto transport = std::make_shared<NetworkingTransportASIO>(connection.get());
                        m_incomingConnections.push_back(connection);
                        m_context.m_successCallback(transport);
                    });
                    result = true;
                }
            }
        };
        if (!result && failCallback)
        {
            failCallback();
        }
        return result;
    }

    bool NetworkingListenerASIO::stop()
    {
        // TODO
        return false;
    }

    void NetworkingListenerASIO::setOnAuthorization(nau::Functor<bool(const INetworkingIdentity&, const NetworkingAddress&)>)
    {
        // TODO
    }

    NetworkingConnectorASIO::NetworkingConnectorASIO(asio::io_context& io_context) :
        m_context(io_context)
    {
    }

    NetworkingConnectorASIO::~NetworkingConnectorASIO() = default;

    void NetworkingConnectorASIO::setOnAuthorization(nau::Functor<bool(const INetworkingIdentity&, const NetworkingAddress&)>)
    {
        // TODO
    }

    bool NetworkingConnectorASIO::connect(const std::string& uriStr,
                                          nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback,
                                          nau::Functor<void(void)> failCallback)
    {
        bool result = false;
        std::string scheme;
        std::string host;
        std::string port;
        if (NetworkingASIO::ParseURI(uriStr, scheme, host, port))
        {
            if (scheme == "tcp")
            {
                m_context.m_successCallback = std::move(successCallback);
                m_context.m_failCallback = std::move(failCallback);
                m_connection = std::make_shared<ASIO_Connection>(m_context.m_io_context);
                auto endpoint = tcp::endpoint(ip::address::from_string(host.c_str()), std::stoi(port.c_str()));
                m_connection->connect(endpoint, [this](std::error_code ec) -> void
                {
                    if (!ec)
                    {
                        auto transport = std::make_shared<NetworkingTransportASIO>(m_connection.get());
                        m_context.m_successCallback(transport);
                    }
                    else
                    {
                        m_context.m_failCallback();
                    }
                });
                result = true;
            }
        };
        if (!result && failCallback)
        {
            failCallback();
        }
        return result;
    }

    bool NetworkingConnectorASIO::connect(const std::string& uri,
                                          nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback,
                                          nau::Functor<void(void)> failCallback,
                                          nau::Functor<void(std::shared_ptr<INetworkingSignaling>)> signalingCallback)
    {
        // No P2P in ASIO
        if (failCallback)
        {
            failCallback();
        }
        return false;
    }

    bool NetworkingConnectorASIO::stop()
    {
        // TODO
        return false;
    }

}  // namespace nau
