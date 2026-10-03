// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// inetworking_transport.h

#pragma once
#include <string>
#include <vector>

#include "networking_message.h"

namespace nau
{
    /**
     * @brief Provides an interface for data transfer.
     */
    class INetworkingTransport
    {
    public:

        /**
         * @brief Attempts to read received messages.
         * 
         * @param [out] messages    A collection of received message. Empty if no messages.
         * @return                  Number of received messages, i.e. size of **messages**.
         */
        virtual size_t read(std::vector<nau::NetworkingMessage>& messages) = 0;

        /**
         * @brief Attempts to send the specified message.
         * 
         * @param [in] message  Message to send.
         * @return              `true` if the message can be sent, `false` otherwise.
         */
        virtual bool write(const nau::NetworkingMessage& message) = 0;

        /**
         * @brief Checks if the connection is alive.
         * 
         * @return `true` if the connection is still alive, `false` otherwise.
         */
        virtual bool isConnected() = 0;

        /**
         * @brief Drops the connection.
         * 
         * @return `true` if the connections has been dropped successfully, `false` otherwise.
         */
        virtual bool disconnect() = 0;

        /**
         * @brief Retrieves the local endpoint as URI.
         * 
         * @return Local endpoint in URI form.
         */
        virtual const std::string& localEndPoint() const = 0;

        /**
         * @brief Retrieves the remote endpoint as URI.
         * 
         * @return Remote endpoint in URI form.
         */
        virtual const std::string& remoteEndPoint() const = 0;
    };
}  // namespace nau
