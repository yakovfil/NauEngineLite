// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>

#include <cstring>

#include "../src/core/ecsQueryInternal.h"
#include "daECS/core/baseIo.h"
#include "daECS/core/componentTypes.h"
#include "daECS/core/entityManager.h"
#include "daECS/core/sharedComponent.h"
#include "daECS/core/template.h"
#include "nau/app/application_services.h"
#include "nau/diag/logging.h"
#include "nau/service/service_provider.h"

class EcsStandardContainers : public testing::Test
{
    std::unique_ptr<nau::Application> app;
    void SetUp() override
    {
        app = nau::createApplication();
        ASSERT_TRUE(app);
        app->startupOnCurrentThread();
        g_entity_mgr.demandInit();
    }
    void TearDown() override
    {
        g_entity_mgr.demandDestroy();
        app->stop();
        while (app->step())
            std::this_thread::yield();
        app.reset();
    }
};
TEST_F(EcsStandardContainers, BooleanAndStringListsKeepAddressableValues)
{
    ecs::BoolList flags;
    for (int i = 0; i < 130; ++i)
        flags.push_back(i % 3 == 0);
    bool& item = flags[64];
    item = true;
    EXPECT_EQ(&item, flags.data() + 64);
    auto copied = flags;
    flags.clear();
    ASSERT_EQ(copied.size(), 130u);
    EXPECT_TRUE(copied[64]);
    for (int i = 0; i < 130; ++i)
        EXPECT_EQ(copied[i], i == 64 || i % 3 == 0);
    ecs::StringList strings;
    for (int i = 0; i < 256; ++i)
        strings.push_back(std::string(i % 2 ? 3 : 128, 'x'));
    auto moved = std::move(strings);
    ASSERT_EQ(moved.size(), 256u);
    for (int i = 0; i < 256; ++i)
        EXPECT_EQ(moved[i], std::string(i % 2 ? 3 : 128, 'x'));
}
TEST_F(EcsStandardContainers, ComponentOrderingAndSharedIdentitySurviveGrowth)
{
    ecs::ComponentsMap values;
    for (uint32_t i = 100; i != 0; --i)
        values[i] = std::string(i % 2 ? "small" : std::string(100, 'z'));
    uint32_t expected = 1;
    for (const auto& [key, value] : values)
    {
        EXPECT_EQ(key, expected++);
        EXPECT_TRUE(value.is<ecs::string>());
    }
    auto copy = values;
    EXPECT_EQ(copy, values);
    values = ecs::ComponentsMap{};
    EXPECT_EQ(copy.size(), 100u);
    ecs::SharedComponent<ecs::string> owner(std::string("shared owner"));
    auto* ownerSlot = &owner;
    decltype(owner)* shared = nullptr;
    ecs::SharedComponentCreator<ecs::string> creator;
    ASSERT_TRUE(creator.copy(&shared, &ownerSlot, ecs::INVALID_COMPONENT_INDEX, ecs::EntityId{}));
    EXPECT_EQ(owner.get(), shared->get());
    owner.reset();
    EXPECT_EQ(**shared, "shared owner");
    EXPECT_EQ(shared->use_count(), 1);
    creator.destroy(&shared);
}
namespace
{
    struct Bytes : ecs::SerializerCb,
                   ecs::DeserializerCb
    {
        std::vector<unsigned char> bytes;
        mutable size_t cursor = 0;
        void write(const void* data, size_t bits, ecs::component_type_t) override
        {
            ASSERT_EQ(bits % 8, 0u);
            const auto* first = static_cast<const unsigned char*>(data);
            bytes.insert(bytes.end(), first, first + bits / 8);
        }
        bool read(void* data, size_t bits, ecs::component_type_t) const override
        {
            if (bits % 8 || bits / 8 > bytes.size() - cursor)
                return false;
            std::memcpy(data, bytes.data() + cursor, bits / 8);
            cursor += bits / 8;
            return true;
        }
    };
}  // namespace
TEST_F(EcsStandardContainers, ComponentSerializationRetainsBytesAndValues)
{
    Bytes compressed;
    ecs::write_compressed(compressed, 300);
    EXPECT_EQ(compressed.bytes, (std::vector<unsigned char>{0xac, 0x02}));
    uint32_t decoded = 0;
    ASSERT_TRUE(ecs::read_compressed(compressed, decoded));
    EXPECT_EQ(decoded, 300u);
    for (const auto& text : {std::string("short"), std::string(1024, 'x')})
    {
        ecs::ChildComponent original;
        original = text;
        Bytes serialized;
        ecs::serialize_child_component(original, serialized);
        auto restored = ecs::deserialize_child_component(serialized);
        ASSERT_TRUE(restored.has_value());
        EXPECT_EQ(restored->get<ecs::string>(), text);
        Bytes rewritten;
        ecs::serialize_child_component(*restored, rewritten);
        EXPECT_EQ(rewritten.bytes, serialized.bytes);
    }
}

TEST_F(EcsStandardContainers, SharedLazyCreationAndBoxedEntityGrowth)
{
    static_assert(ecs::ComponentTypeInfo<ecs::string>::is_boxed);
    static_assert(ecs::ComponentTypeInfo<ecs::StringList>::is_boxed);
    static_assert(ecs::ComponentTypeInfo<ecs::SharedComponent<ecs::string>>::is_boxed);
    ecs::SharedComponentCreator<ecs::string> creator;
    ecs::SharedComponent<ecs::string>* original = nullptr;
    ecs::SharedComponent<ecs::string>* copied = nullptr;
    creator.create(&original, *g_entity_mgr, ecs::EntityId{}, {}, ecs::INVALID_COMPONENT_INDEX);
    ASSERT_FALSE(*original);
    ASSERT_TRUE(creator.copy(&copied, &original, ecs::INVALID_COMPONENT_INDEX, ecs::EntityId(1)));
    ASSERT_TRUE(*original);
    EXPECT_EQ(original->get(), copied->get());
    **original = std::string(256, 's');
    creator.destroy(&original);
    EXPECT_EQ(**copied, std::string(256, 's'));
    creator.destroy(&copied);

    ecs::ComponentsMap components;
    components[ECS_HASH("short_text")] = std::string("short");
    components[ECS_HASH("long_text")] = std::string(256, 'x');
    ecs::StringList list;
    list.push_back(std::string("tiny"));
    list.push_back(std::string(256, 'l'));
    components[ECS_HASH("list")] = list;
    components[ECS_HASH("shared")] = ecs::SharedComponent<ecs::string>(std::string("identity"));
    g_entity_mgr->addTemplate(ecs::Template("std_lifetimes", std::move(components), {}, {}, {}, false));
    std::vector<ecs::EntityId> entities;
    for (int i = 0; i < 512; ++i)
    {
        auto entity = g_entity_mgr->createEntitySync("std_lifetimes");
        ASSERT_TRUE(entity);
        entities.push_back(entity);
    }
    const auto* identity = g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(entities.front(), ECS_HASH("shared")).get();
    for (auto entity : entities)
    {
        EXPECT_EQ(g_entity_mgr->get<ecs::string>(entity, ECS_HASH("short_text")), "short");
        EXPECT_EQ(g_entity_mgr->get<ecs::string>(entity, ECS_HASH("long_text")), std::string(256, 'x'));
        EXPECT_EQ(g_entity_mgr->get<ecs::StringList>(entity, ECS_HASH("list")), list);
        EXPECT_EQ(g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(entity, ECS_HASH("shared")).get(), identity);
    }
    for (size_t i = 0; i < entities.size(); i += 2)
        g_entity_mgr->destroyEntityAsync(entities[i]);
    g_entity_mgr->tick();
    for (size_t i = 1; i < entities.size(); i += 2)
    {
        EXPECT_EQ(g_entity_mgr->get<ecs::string>(entities[i], ECS_HASH("short_text")), "short");
        EXPECT_EQ(g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(entities[i], ECS_HASH("shared")).get(), identity);
        g_entity_mgr->destroyEntityAsync(entities[i]);
    }
    g_entity_mgr->tick();
}

TEST_F(EcsStandardContainers, QueryGrowthPreservesInlineUnionsAndOwnedTracking)
{
    static_assert(!dag::is_type_relocatable<ecs::ArchetypesQuery>::value);
    std::vector<ecs::ArchetypesQuery> queries;
    for (int row = 0; row < 64; ++row)
    {
        ecs::ArchetypesQuery query;
        query.roCount = 2;
        query.rwCount = 0;
        query.queriesCount = 7;
        query.firstArch = 1;
        query.secondArch = 2;
        query.thirdArch = 3;
        for (int i = 0; i < 4; ++i)
            query.arches[i] = static_cast<ecs::archetype_t>(i + 4);
        for (int i = 0; i < 14; ++i)
            query.offsets[i] = static_cast<uint16_t>(row * 16 + i);
        query.trackedChangesCount = 1;
        auto* track = static_cast<ecs::ScheduledArchetypeComponentTrack*>(std::malloc(sizeof(ecs::ScheduledArchetypeComponentTrack)));
        std::construct_at(track, static_cast<ecs::archetype_t>(row), static_cast<ecs::component_index_t>(row + 1));
        query.trackedChanges.reset(track);
        queries.push_back(std::move(query));
    }
    for (int row = 0; row < 64; ++row)
    {
        const auto& query = queries[row];
        for (int i = 0; i < 4; ++i)
            EXPECT_EQ(query.arches[i], i + 4);
        for (int i = 0; i < 14; ++i)
            EXPECT_EQ(query.offsets[i], row * 16 + i);
        EXPECT_EQ(query.trackedBegin()->archetype, row);
        EXPECT_EQ(query.trackedBegin()->cidx, row + 1);
    }
}

int main(int argc, char** argv)
{
    nau::diag::setLogger(nau::diag::createLogger());
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
