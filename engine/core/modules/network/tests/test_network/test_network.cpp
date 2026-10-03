// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <map>
#include <string>

#include "nau/components/net_component_api.h"
#include "nau/diag/logging.h"
#include "nau/io/stream_utils.h"
#include "nau/math/math.h"
#include "nau/memory/heap_allocator.h"
#include "nau/napi/networking.h"
#include "nau/scene/components/component.h"
#include "nau/scene/nau_object.h"
#include "nau/serialization/json.h"
#include "nau/serialization/json_utils.h"
#include "nau/serialization/runtime_value_builder.h"
#include "nau/string/string_conv.h"

namespace nau
{
    struct NetworkComponentData
    {
        math::vec3 vec = math::vec3::zero();
        math::quat rot = math::quat::identity();

        NAU_CLASS_FIELDS(
            CLASS_FIELD(vec),
            CLASS_FIELD(rot))

        auto operator<=>(const NetworkComponentData&) const = default;

        bool write(std::u8string& buffer)
        {
            io::InplaceStringWriter<char8_t> writer{buffer};
            serialization::JsonSettings settings;
            auto res = serialization::jsonWrite(writer, makeValueRef(*this, getDefaultAllocator()), settings);
            return res.isSuccess();
        }

        bool read(const std::u8string& buffer)
        {
            Result<RuntimeValue::Ptr> parseResult = serialization::jsonParseString(buffer, getDefaultAllocator());
            auto root = serialization::jsonParseToValue(buffer);
            if (!root.isError())
            {
                if (parseResult)
                {
                    auto res = RuntimeValue::assign(makeValueRef(*this), *parseResult);
                    return res.isSuccess();
                }
            }
            return false;
        }
    };

    class NetworkComponentBase : public scene::Component,
                                 public IComponentNetSync
    {
        NAU_OBJECT(NetworkComponentBase, scene::Component, IComponentNetSync)

        NAU_DECLARE_DYNAMIC_OBJECT

    public:
        const char* getSceneName() override
        {
            return nullptr;
        }

        const char* getComponentPath() override
        {
            return nullptr;
        }

        virtual void netWrite(BytesBuffer& buffer)
        {
        }

        virtual void netRead(const BytesBuffer& buffer)
        {
        }

        virtual void netWrite(std::string& buffer)
        {
        }

        virtual void netRead(const std::string& buffer)
        {
        }
    };

    class NetworkComponentInter : public NetworkComponentBase
    {
        NAU_OBJECT(NetworkComponentInter, NetworkComponentBase)

        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_BASE(NetworkComponentBase)

    public:
    };

    class NetworkComponentTest final : public NetworkComponentInter
    {
        NAU_OBJECT(NetworkComponentTest, NetworkComponentInter)

        NAU_DECLARE_DYNAMIC_OBJECT

        NAU_CLASS_BASE(NetworkComponentInter)

        NAU_CLASS_FIELDS(
            CLASS_NAMED_FIELD(m_data, "data"))

    public:
        NetworkComponentData m_data;
    };

    NAU_IMPLEMENT_DYNAMIC_OBJECT(NetworkComponentBase);
    NAU_IMPLEMENT_DYNAMIC_OBJECT(NetworkComponentInter);
    NAU_IMPLEMENT_DYNAMIC_OBJECT(NetworkComponentTest);

    class SnapshotTest
    {
    public:
        NAU_CLASS_FIELDS(
            CLASS_FIELD(frameSnapshot))

        struct ComponentData
        {
            std::string m_data;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(m_data))
        };

        struct SceneSnapshot
        {
            std::map<std::string, ComponentData> m_components;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(m_components))
        };

        struct FrameSnapshot
        {
            std::map<std::string, SceneSnapshot> m_scenes;

            NAU_CLASS_FIELDS(
                CLASS_FIELD(m_scenes))
        };

        FrameSnapshot frameSnapshot;
    };

}  // namespace nau

namespace nau::test
{
    TEST(TestNetwork, TestTemplate)
    {
        ASSERT_TRUE(true);
    }

    TEST(TestNetwork, GetFields)
    {
        const NetworkComponentData instance;

        auto fields = meta::getClassAllFields<decltype(instance)>();
        ASSERT_TRUE(true);
    }

    TEST(TestNetwork, TestComponentData)
    {
        NetworkComponentData componentTestSrc;
        NetworkComponentData componentTestDst;
        std::u8string buffer;

        componentTestSrc.vec = math::vec3(1, 2, 3);
        componentTestSrc.rot = math::quat(10, 20, 30);

        auto writeRes = componentTestSrc.write(buffer);
        ASSERT_TRUE(writeRes);

        auto readRes = componentTestDst.read(buffer);
        ASSERT_TRUE(readRes);

        ASSERT_EQ(componentTestSrc.vec.getX(), componentTestDst.vec.getX());
        ASSERT_EQ(componentTestSrc.vec.getY(), componentTestDst.vec.getY());
        ASSERT_EQ(componentTestSrc.vec.getZ(), componentTestDst.vec.getZ());

        ASSERT_EQ(componentTestSrc.rot.getX(), componentTestDst.rot.getX());
        ASSERT_EQ(componentTestSrc.rot.getY(), componentTestDst.rot.getY());
        ASSERT_EQ(componentTestSrc.rot.getZ(), componentTestDst.rot.getZ());
        ASSERT_EQ(componentTestSrc.rot.getW(), componentTestDst.rot.getW());

        ASSERT_TRUE(true);
    }

    TEST(TestNetwork, TestSnapshot)
    {
        // ComponentData
        NetworkComponentTest componentTestSrc;
        SnapshotTest::ComponentData componentData;
        SnapshotTest::SceneSnapshot sceneSnapshot;
        SnapshotTest snapshotTestSrc;
        SnapshotTest snapshotTestDst;

        componentTestSrc.m_data.vec = math::vec3(1, 2, 3);
        componentTestSrc.m_data.rot = math::quat(10, 20, 30);
        componentTestSrc.netWrite(componentData.m_data);
        sceneSnapshot.m_components.emplace("Component1", componentData);
        snapshotTestSrc.frameSnapshot.m_scenes.emplace("Scene1", sceneSnapshot);

        auto str = serialization::JsonUtils::stringify<char>(snapshotTestSrc);

        auto res = serialization::JsonUtils::parse(snapshotTestDst, str.c_str());

        ASSERT_EQ(snapshotTestSrc.frameSnapshot.m_scenes.size(), snapshotTestDst.frameSnapshot.m_scenes.size());
        ASSERT_EQ(snapshotTestSrc.frameSnapshot.m_scenes["Scene1"].m_components.size(), snapshotTestDst.frameSnapshot.m_scenes["Scene1"].m_components.size());
        auto& comp1 = snapshotTestSrc.frameSnapshot.m_scenes["Scene1"].m_components["Component1"].m_data;
        auto& comp2 = snapshotTestDst.frameSnapshot.m_scenes["Scene1"].m_components["Component1"].m_data;
        ASSERT_STREQ(comp1.c_str(), comp2.c_str());

        ASSERT_TRUE(true);
    }
}  // namespace nau::test
