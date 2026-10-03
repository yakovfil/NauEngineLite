// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// inetworking_connection.h

#pragma once
#include <nau/utils/functor.h>

#include <memory>
#include <string>

#include "networking_address.h"
#include "networking_signaling.h"
#include "networking_transport.h"
#include "networkinig_identity.h"

namespace nau
{
    /**
     * @brief Provides basic interface for a connection object.
     */
    class INetworkingConnection
    {
    public:
        /**
         * @brief Sets the callback that is dispatched when a connection attempts to authorize.
         * 
         * @param [in] cb Callback to assign.
         * 
         * @note The callback takes the identity to check and the address of incoming connection as parameters.
         */
        virtual void setOnAuthorization(nau::Functor<bool(const INetworkingIdentity& identity, const NetworkingAddress& address)> cb) = 0;
    };

    /**
     * @brief Provides an interface for establishing a network connection.
     */
    class INetworkingConnector : public INetworkingConnection
    {
    public:

        /**
         * @brief Connects using URI with callbacks.
         * 
         * @param [in] uri              URI for the remote endpoint.
         * @param [in] successCallback  Callback that is dispatched, if the connection has been successful.
         * @param [in] failCallback     Callback that is dispatched, if the connection has failed.
         * @return                      `true`, if connection attempt has started, `false` otherwise.
         */
        virtual bool connect(const std::string& uri,
                             nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback,
                             nau::Functor<void(void)> failCallback) = 0;

        /**
         * @brief Connects using URI with callbacks.
         * 
         * @param [in] uri                  URI for remote endpoint.
         * @param [in] successCallback      Callback that is dispatched, if the connection has been successful.
         * @param [in] failCallback         Callback that is dispatched, if the connection has failed.
         * @param [in] signalingCallback    Callback to signaling service.
         * @return                          `true`, if connection attempt has started, `false` otherwise.
         */
        virtual bool connect(const std::string& uri,
                             nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback,
                             nau::Functor<void(void)> failCallback,
                             nau::Functor<void(std::shared_ptr<INetworkingSignaling>)> signalingCallback) = 0;

        /**
         * @brief Stops connection attempts.
         * 
         * @return `true`, if connection attempts has been stopped, `false` otherwise.
         */
        virtual bool stop() = 0;
    };

    /**
     * @brief Provides an interface for listening to a network connection.
     */
    class INetworkingListener : public INetworkingConnection
    {
    public:
        /**
         * @brief Listens to the connection.
         * 
         * @param [in] uri              URI for local endpoint.
         * @param [in] successCallback  Callback that is dispatched, if the connection has succeeded.
         * @param [in] failCallback     Callback that is dispatched, if the connection has failed.
         * @return                      `true`, if listening has started, `false` otherwise.
         */
        virtual bool listen(const std::string& uri, nau::Functor<void(std::shared_ptr<INetworkingTransport>)> successCallback, nau::Functor<void(void)> failCallback) = 0;

        /**
         * @brief Stops listening.
         * 
         * @return `true`, if listening has stopped, `false` otherwise.
         */
        virtual bool stop() = 0;
    };
}  // namespace nau
