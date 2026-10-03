// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// inetworkinig_identity.h

#pragma once
#include <string>

namespace nau
{
    /**
     * @brief Enumerates known ID types.
     */
    enum class NetworkingIdentityType : unsigned
    {
        Unknown,  ///< Not set.
        Local,    ///< Set, but not confirmed by the external service.
        Nau,
        SteamID,
        XboxPairwiseID,
        SonyPSN
    };

    /**
     * @brief Provides an interface for networking ID of some type.
     */
    class INetworkingIdentity
    {
    public:
        /**
         * @brief Retrieves the networking ID Type.
         * 
         * @return Networking ID type.
         */
        virtual NetworkingIdentityType getType() const = 0;

        /**
         * @brief Retrieves the networking ID text representation.
         * 
         * @return Networking ID text representation
         */
        virtual std::string toString() const = 0;
    };
}  // namespace nau
