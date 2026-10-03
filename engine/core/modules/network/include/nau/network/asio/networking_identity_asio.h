// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// networking_identity_asio

#pragma once
#include <string>

#include "nau/network/napi/networkinig_identity.h"

namespace nau
{

    /**
     * @brief Encapsulates ASIO-specific ID.
     */
    class NetworkingIdentityASIO : public INetworkingIdentity
    {
    public:

        /**
         * @brief Initialization constructor.
         * 
         * @param [in] id String networking id.
         */
        NetworkingIdentityASIO(const std::string& id) :
            m_dentity(id)
        {
        }

        ~NetworkingIdentityASIO() = default;

        /**
         * @brief Retrieves the networking ID Type.
         *
         * @return Networking ID type.
         */
        NetworkingIdentityType getType() const override
        {
            return NetworkingIdentityType::Local;
        }

        /**
         * @brief Retrieves the networking ID text representation.
         *
         * @return Networking ID text representation
         */
        std::string toString() const
        {
            return m_dentity;
        }

    private:
        std::string m_dentity;
    };
}  // namespace nau
