// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <memory>
#include <string>

#include "../src/networking_factory_impl.h"
#include "nau/asio/networking_asio.h"
#include "nau/napi/networking.h"

namespace nau
{
    namespace test
    {
        TEST(TestNetTransportASIO, TestTemplate)
        {
            ASSERT_TRUE(true);
        }

        TEST(TestNetTransportASIO, TestCreate)
        {
            NetworkingFactoryImpl::Register("ASIO", NetworkingASIO::create);
            auto ptr = NetworkingFactoryImpl::Create("ASIO");
            ASSERT_TRUE(ptr);
        }

        TEST(TestNetTransportASIO, TestConnection)
        {
            NetworkingFactoryImpl::Register("ASIO", NetworkingASIO::create);
            auto netPtr = NetworkingFactoryImpl::Create("ASIO");
            ASSERT_TRUE(netPtr);
            auto listener = netPtr->createListener();
            auto connector = netPtr->createConnector();
            std::string url("tcp://127.0.0.1:9994/");
            int listenerState = 0;
            int connectorState = 0;
            listener->listen(
                url,
                [&listenerState](std::shared_ptr<INetworkingTransport> incomingTransport) -> void
            {
                listenerState = 1;
            },
                [&listenerState]() -> void
            {
                listenerState = 2;
            });
            connector->connect(
                url,
                [&connectorState](std::shared_ptr<INetworkingTransport> incomingTransport) -> void
            {
                connectorState = 1;
            },
                [&connectorState]() -> void
            {
                connectorState = 2;
            });
            netPtr->update();
            ASSERT_TRUE(listenerState == 1);
            ASSERT_TRUE(connectorState == 1);
        }

    }  // namespace test
}  // namespace nau
