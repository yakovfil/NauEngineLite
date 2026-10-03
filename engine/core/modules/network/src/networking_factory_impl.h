// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <map>
#include <memory>
#include <string>

#include "nau/network/napi/networking_factory.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/rtti/rtti_object.h"
#include "nau/service/service.h"

namespace nau
{

    /**
    @brief Factory, register implementations and create INetworking instances
    */
    class NetworkingFactoryImpl final : public IServiceInitialization,
                                        public NetworkingFactory
    {
        NAU_RTTI_CLASS(nau::NetworkingFactoryImpl, NetworkingFactory)

    public:
        virtual async::Task<> preInitService() override;
        virtual async::Task<> initService() override;

        virtual std::unique_ptr<INetworking> create(const std::string& name) override;

        using TCreateMethod = std::unique_ptr<INetworking> (*)();
        TCreateMethod m_CreateFunc;

        /**
        @brief Singleton, register
        */
        static bool Register(const std::string name, TCreateMethod createFunc)
        {
            if (auto it = s_methods.find(name); it == s_methods.end())
            {
                s_methods[name] = createFunc;
                return true;
            }
            return false;
        }

        /**
        @brief Singleton, create
        */
        static std::unique_ptr<INetworking> Create(const std::string& name)
        {
            if (auto it = s_methods.find(name); it != s_methods.end())
                return it->second();

            return nullptr;
        }

    private:
        static std::map<std::string, TCreateMethod> s_methods;
    };
}  // namespace nau
