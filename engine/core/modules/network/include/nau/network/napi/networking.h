// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// inetworking.h

#pragma once
#include <memory>
#include <string>

#include "networking_connection.h"
#include "networking_signaling.h"
#include "networking_transport.h"
#include "networkinig_identity.h"

namespace nau
{
    /**
     *  @brief Provides interface for a network context instance.
     */
    class INetworking
    {
    public:
        /**
         * @brief Apply config string
         *
         * @param [in] data String data in implementation dependent format.
         * @return          `true`, if the string data has been successfully parsed and applied, `false` otherwise.
         */
        virtual bool applyConfig(const std::string& data) = 0;

        /**
         * @brief Initializes networking context. Call this function once before the first update.
         * 
         * @return `true` on success, `false` otherwise.
         */
        virtual bool init() = 0;

        /**
         * @brief Shuts down the context and frees all resources.
         * 
         * @return `true` on success, `false` otherwise.
        */
        virtual bool shutdown() = 0;

        /**
         * @brief Updates state (polling). This function must be called in a loop.
         * 
         * @return `true` on success, `false` otherwise.
         */
        virtual bool update() = 0;

        /**
         * @brief Retrieves the context instance id. It has to be unique for each instance in each process.
         * 
         * @return A reference to a INetworkingIdentity object.
         */
        virtual const INetworkingIdentity& identity() const = 0;

        /**
         * @brief Creates a listener object.
         * 
         * @return A pointer to the INetworkingListener instance or `NULL` in case of failure.
         */
        virtual std::shared_ptr<INetworkingListener> createListener() = 0;

        /**
         * @brief Creates a connector object. 
         * 
         * @return A pointer to the INetworkingConnector instance or `NULL` in case of failure.
         */
        virtual std::shared_ptr<INetworkingConnector> createConnector() = 0;
    };
}  // namespace nau
