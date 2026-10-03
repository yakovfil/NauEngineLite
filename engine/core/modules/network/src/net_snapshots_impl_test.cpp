// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// net_snapshots_impl.cpp

#include "net_snapshots_impl.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "nau/diag/logging.h"
#include "nau/network/napi/networking_factory.h"
#include "nau/serialization/json.h"
#include "nau/serialization/json_utils.h"
#include "nau/service/service_provider.h"

namespace nau
{
    bool NetSnapshotsImpl::doSelfTest()
    {
        class TestSceneComponent : public IComponentNetScene
        {
        public:
            TestSceneComponent(const char* peerId, const char* sceneName) :
                m_peerId(peerId),
                m_sceneName(sceneName)
            {
            }

            virtual std::string_view getPeerId() override
            {
                return m_peerId;
            }

            virtual std::string_view getSceneName() override
            {
                return m_sceneName;
            }

            virtual IComponentNetSync* getOrCreateComponent(std::string_view path, std::string_view type) override
            {
                return nullptr;
            }

            const char* m_peerId;
            const char* m_sceneName;
        };

        class TestSyncComponent : public IComponentNetSync
        {
        public:
            TestSyncComponent(const char* componentPath, const char* sceneName) :
                m_componentPath(componentPath),
                m_sceneName(sceneName)
            {
            }

            virtual std::string_view getSceneName() override
            {
                return m_sceneName;
            }

            virtual std::string_view getComponentPath() override
            {
                return m_componentPath;
            }

            virtual void setIsReplicated(bool isReplicated) override
            {
            }

            /**
            @brief Is component owned or replicated
            */
            virtual bool isReplicated() const override
            {
                return true;
            }

            // Binary serialization
            virtual void netWrite(BytesBuffer& buffer) override
            {
            }
            virtual void netRead(const BytesBuffer& buffer) override
            {
            }

            // Text (JSON) serialization
            virtual void netWrite(std::string& buffer) override
            {
                buffer = "TestSyncComponentSerialized";
            }

            virtual void netRead(const std::string& buffer) override
            {
            }

            const char* m_componentPath;
            const char* m_sceneName;
        };

        m_peers.clear();
        const char* peerName = "Peer1";
        const char* sceneName = "Scene1";
        const char* componentPath = "root/c1";
        auto res1 = m_peers.emplace(peerName, PeerData());
        auto& peer1 = (*res1.first).second;
        peer1.advanceToFrame(1);
        TestSceneComponent tsc1(peerName, sceneName);
        peer1.activateScene(&tsc1);
        TestSyncComponent tsyc1(componentPath, sceneName);
        peer1.writeComponent(sceneName, &tsyc1);

        std::string frameBuffer;
        peer1.serializeFrame(1, frameBuffer);

        //
        std::unique_ptr<INetworking> networking;
        std::shared_ptr<INetworkingListener> listener;
        std::shared_ptr<INetworkingConnector> connector;
        std::shared_ptr<nau::INetworkingTransport> transportIncoming;
        std::shared_ptr<nau::INetworkingTransport> transportOutgoing;

        auto service = nau::getServiceProvider().find<NetworkingFactory>();
        networking = service->create("ASIO");
        networking->init();

        listener = networking->createListener();
        listener->listen(
            "tcp://127.0.0.1:9999/",
            [&transportIncoming](std::shared_ptr<nau::INetworkingTransport> incomingTransport) -> void
        {
            transportIncoming = incomingTransport;
        },
            []() -> void
        {
        });

        connector = networking->createConnector();
        connector->connect(
            "tcp://127.0.0.1:9999/",
            [&transportOutgoing](std::shared_ptr<nau::INetworkingTransport> outgoingTransport) -> void
        {
            transportOutgoing = outgoingTransport;
        },
            []() -> void
        {
        });

        networking->update();

        transportOutgoing->write(NetworkingMessage(frameBuffer));
        networking->update();

        std::vector<nau::NetworkingMessage> messages;
        transportIncoming->read(messages);
        if (messages.size() > 0)
        {
            auto res2 = m_peers.emplace("Peer2", PeerData());
            auto& peer2 = (*res2.first).second;

            const char* ptr = (const char*)messages[0].buffer.data();
            std::string incomingFrameBuffer(ptr, ptr + messages[0].buffer.size());

            //

            peer2.deserializeFrame(incomingFrameBuffer);
            return peer2.m_frames.size() == peer1.m_frames.size();
        }

        m_peers.clear();
        return false;
    }
}  // namespace nau
