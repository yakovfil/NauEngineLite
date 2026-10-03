// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// inetworking_signaling.h

#pragma once
#include <string>

namespace nau
{
    /**
     * @brief Provides an interface for a side channel for P2P connections.
     */
    class INetworkingSignaling
    {
    public:
        /**
         * @brief Sends the signal to the side channel.
         * 
         * @param [in] identity Destination identity.
         * @param [in] data     Data to send.
         */
        virtual void send(const std::string& identity, const std::string& data) = 0;

        /**
         * @brief Sets the callback that is used to filter data from the side channel.
         * 
         * @param [in] cb Callback to assign.
         * 
         * @note The callback takes the source directory and a reference to the received data as parameters.
         */
        virtual void setOnDispatch(nau::Functor<bool(const std::string& fromIdentity, const std::string& data)> cb) = 0;
    };
}  // namespace nau
