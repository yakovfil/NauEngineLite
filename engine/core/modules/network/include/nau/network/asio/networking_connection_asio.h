// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// networking_connection_asio

#pragma once
#include <asio.hpp>
#include <memory>
#include <string>
#include <vector>

#include "nau/network/napi/networking_connection.h"

namespace nau
{
    class ASIO_Acceptor;
    class ASIO_Connection;

    struct NetworkingConectionASIOContext
    {
        asio::io_context& m_io_context;
        nau::Functor<void(std::shared_ptr<INetworkingTransport>)> m_successCallback;
        nau::Functor<void(void)> m_failCallback;
        NetworkingConectionASIOContext(asio::io_context& context) :
            m_io_context(context)
        {
        }
    };

    /**
    @brief Class for listen for connection
    */
    class NetworkingListenerASIO : public INetworkingListener
    {
    public:
        NetworkingListenerASIO(asio::io_context& io_context);
        ~NetworkingListenerASIO();

        /**
         * @brief Sets the callback that is dispatched when a connection attempts to authorize.
         * 
         * @param [in] cb Callback to assign.
         * 
         * @note The callback takes the identity to check and the address of incoming connection as parameters.
         */
        void setOnAuthorization(nau::Functor<bool(const INetworkingIdentity& identity, const NetworkingAddress&)> address) override;

        // TODO - TaskINetworkingTransport::Ptr
        /**
        @brief Listen for connection
        @param [in] uri URI for local endpoint
        @param [in] successCallback Callback, if connection succeeded
        @param [in] failCallback Callback, if connection failed
        @return True, if listening begin, false otherwise
        */
        bool listen(const std::string& uri, nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback, nau::Functor<void(void)> failCallback) override;

        /**
        @brief Stop listen
        @return True if stopped, false otherwise
        */
        bool stop() override;

    private:
        NetworkingConectionASIOContext m_context;
        std::shared_ptr<ASIO_Acceptor> m_acceptor;
        std::vector<std::shared_ptr<ASIO_Connection>> m_incomingConnections;
    };

    /**
     * @brief Establishing a ASIO-based network connection.
     */
    class NetworkingConnectorASIO : public INetworkingConnector
    {
    public:
        NetworkingConnectorASIO(asio::io_context& io_context);
        ~NetworkingConnectorASIO();

        /**
         * @brief Sets the callback that is dispatched when a connection attempts to authorize.
         *
         * @param [in] cb Callback to assign.
         *
         * @note The callback takes the identity to check and the address of incoming connection as parameters.
         */
        void setOnAuthorization(nau::Functor<bool(const INetworkingIdentity& identity, const NetworkingAddress&)> address) override;

        /**
         * @brief Connects using URI with callbacks.
         *
         * @param [in] uri              URI for the remote endpoint.
         * @param [in] successCallback  Callback that is dispatched, if the connection has been successful.
         * @param [in] failCallback     Callback that is dispatched, if the connection has failed.
         * @return                      `true`, if connection attempt has started, `false` otherwise.
         */
        bool connect(const std::string& uri,
                     nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback,
                     nau::Functor<void(void)> failCallback) override;
        /**
         * @brief Connects using URI with callbacks.
         *
         * @param [in] uri                  URI for remote endpoint.
         * @param [in] successCallback      Callback that is dispatched, if the connection has been successful.
         * @param [in] failCallback         Callback that is dispatched, if the connection has failed.
         * @param [in] signalingCallback    Callback to signaling service.
         * @return                          `true`, if connection attempt has started, `false` otherwise.
         */
        bool connect(const std::string& uri,
                     nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback,
                     nau::Functor<void(void)> failCallback,
                     nau::Functor<void(std::shared_ptr<INetworkingSignaling>)> signalingCallback) override;

        /**
         * @brief Stops connection attempts.
         *
         * @return `true`, if connection attempts has been stopped, `false` otherwise.
         */
        bool stop() override;

    private:
        NetworkingConectionASIOContext m_context;
        std::shared_ptr<ASIO_Connection> m_connection;
    };

}  // namespace nau
