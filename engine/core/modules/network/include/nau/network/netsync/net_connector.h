// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nau/rtti/type_info.h"

namespace nau
{
    /**
     * @brief Provides an interface for network connector service which manages connections between peers.
     */
    struct NAU_ABSTRACT_TYPE INetConnector
    {
        NAU_TYPEID(INetConnector)

        /**
         * @brief Encapsulates connection information.
         */
        struct ConnectionData
        {
            std::string localPeerId;
            std::string remotePeerId;
            std::string localUri;
        };

        /**
         * @brief Provides access to connection data and state.
         */
        struct IConnection
        {
            /**
             * @brief Enumerates possible connection states.
             */
            enum State
            {
                none,
                accepted,
                connecting,
                connected,
                disconnecting,
                disconnected,
                idRequested,
                ready,
                failed
            };

            /**
             * @brief Retrieves the connection state.
             * 
             * @return Current connection state.
             */
            virtual State state() = 0;

            /**
             * @brief Retrieves the local peer ID. Its representation is implementation-dependent, but it has to be unique for the current sesion,
             * 
             * @return String ID of the local peer.
             */
            virtual const std::string& localPeerId() = 0;

            /**
             * @brief Retrieves the remote peer ID. Its representation is implementation-dependent, but it has to be unique for the current sesion,
             *
             * @return String ID of the remote peer.
             */
            virtual const std::string& remotePeerId() = 0;

            /**
             * @brief Retrieves the local end point URI.
             * 
             * @return Local end point URI.
             */
            virtual const std::string& localEndPoint() = 0;

            /**
             * @brief Retrieves the remote end point URI.
             * 
             * @return Remote end point URI.
             */
            virtual const std::string& remoteEndPoint() = 0;
        };

        /**
         * @brief Initializes the service. The function must be called once.
         */
        virtual void init() = 0;

        /**
         * @brief Starts listening to incoming connection.
         * 
         * @param [in] localPeerId  Local ID of the peer which is awaiting connection.
         * @param [in] remotePeerId Remote ID of the peer which is establishing connection.
         * @param [in] uri          URI that describes protocol, address and port for incoming connection.
         */
        virtual void listen(const std::string& localPeerId, const std::string& remotePeerId, const std::string& uri) = 0;

        /**
         * @brief Retrieves information about active listeners.
         * 
         * @param [out] listeners Data that has been cleared and filled by listeners.
         */
        virtual void getListeners(std::vector<ConnectionData>& listeners) = 0;

        /**
         * @brief Initiates outgoing connection.
         *
         * @param [in] localPeerId  Local ID of the peer which is establishing connection.
         * @param [in] remotePeerId Remote ID of the peer which is awaiting connection.
         * @param [in] uri          URI that describes protocol, address and port for outgoing connection.
         */
        virtual void connect(const std::string& localPeerId, const std::string& remotePeerId, const std::string& uri) = 0;

        /**
         * @brief Retrieves information about active connectors.
         *
         * @param [out] listeners Data that has been cleared and filled by connectors.
         */
        virtual void getConnectors(std::vector<ConnectionData>& connectors) = 0;

        /**
         * @brief Get peers connected to specified peer
         * @param peerId - specific peer
         * @param peers - connected to specific peer peers 
         */
        virtual void getConnections(const std::string& peerId, std::vector<std::string>& peers) = 0;

        /**
         * @brief Get all connections
         * @param connections - cleared and filled by ptrs to established connections
         */
        virtual void getConnections(std::vector<std::weak_ptr<IConnection>>& connections) = 0;

        /**
         * @brief Write frame state, expected to be called once per frame
         * @param peerId - remote peer, destination
         * @param frame - serialized frame state
         */
        virtual void writeFrame(const std::string& peerId, const std::string& frame) = 0;

        /**
         * @brief Read frame state
         * @param peerId - local peer, destination
         * @param fromPeerId - remote peer, source of frame state
         * @param frame - serialized frame state
         * @return True, if frame state exists, false otherwise
         */
        virtual bool readFrame(const std::string& peerId, const std::string& fromPeerId, std::string& frame) = 0;

        /**
         * @brief Update service. Must be called once per frame.
         */
        virtual void update() = 0;
    };
}  // namespace nau
