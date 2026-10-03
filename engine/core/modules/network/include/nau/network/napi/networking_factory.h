// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// inetworking_factory.h

#pragma once

#include <memory>
#include <string>

#include "nau/network/napi/networking.h"
#include "nau/rtti/type_info.h"

namespace nau
{

    /**
     * @brief Network context instance factory class.
     */
    struct NAU_ABSTRACT_TYPE NetworkingFactory
    {
        NAU_TYPEID(nau::NetworkingFactory)

        /**
         * @brief Creates a network context instance using specific implementation.
         * 
         * @param [in] name Name of Network implementation 
         * @return          A pointer to an INetworking instance or `NULL` if creation failed.
         */
        virtual std::unique_ptr<INetworking> create(const std::string& name) = 0;
    };
}  // namespace nau
