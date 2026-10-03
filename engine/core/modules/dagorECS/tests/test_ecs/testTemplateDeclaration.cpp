// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <vector>

#include "test_ecs_common.h"
#include "test_template_declaration.h"

BEGIN_ECS_TEMPLATE(TestTemplate1)
ADD_ECS_COMPONENT(testTemplate1, ecs::Tag)
END_ECS_TEMPLATE()

BEGIN_ECS_TEMPLATE(TestTemplate2, TestTemplate1)
ADD_ECS_COMPONENT(testTemplate2, ecs::Tag)
ADD_ECS_COMPONENT(test_int, int)
END_ECS_TEMPLATE()


namespace nau::test
{

    BEGIN_ECS_TEMPLATE(TestTemplate3, TestTemplate2)
    ADD_ECS_COMPONENT(testTemplate3, ecs::Tag)
    ADD_ECS_COMPONENT(test_struct, TestTemplateDeclarationStruct)
    END_ECS_TEMPLATE()

    TEST_F(TestDagorECS, TemplateDeclaration)
    {
        std::vector<ecs::EntityId> eid;
        auto eid1 = g_entity_mgr->createEntitySync(TestTemplate1::getTemplateId());
        auto eid2 = g_entity_mgr->createEntitySync<TestTemplate1>();
        auto eid3 = g_entity_mgr->createEntitySync(TestTemplate2::getTemplateId());
        auto eid4 = g_entity_mgr->createEntitySync<TestTemplate2>();
        auto eid5 = g_entity_mgr->createEntitySync(TestTemplate3::getTemplateId());
        auto eid6 = g_entity_mgr->createEntitySync<TestTemplate3>();

        TestTemplate1 eid7;
        TestTemplate2 eid8;
        TestTemplate3 eid9;

        ASSERT_TRUE(g_entity_mgr->has(eid1, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid2, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid3, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid4, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid5, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid6, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid7, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid8, ECS_HASH("testTemplate1")));
        ASSERT_TRUE(g_entity_mgr->has(eid9, ECS_HASH("testTemplate1")));

        ASSERT_FALSE(g_entity_mgr->has(eid1, ECS_HASH("testTemplate2")));
        ASSERT_FALSE(g_entity_mgr->has(eid2, ECS_HASH("testTemplate2")));
        ASSERT_TRUE(g_entity_mgr->has(eid3, ECS_HASH("testTemplate2")));
        ASSERT_TRUE(g_entity_mgr->has(eid4, ECS_HASH("testTemplate2")));
        ASSERT_TRUE(g_entity_mgr->has(eid5, ECS_HASH("testTemplate2")));
        ASSERT_TRUE(g_entity_mgr->has(eid6, ECS_HASH("testTemplate2")));
        ASSERT_FALSE(g_entity_mgr->has(eid7, ECS_HASH("testTemplate2")));
        ASSERT_TRUE(g_entity_mgr->has(eid8, ECS_HASH("testTemplate2")));
        ASSERT_TRUE(g_entity_mgr->has(eid9, ECS_HASH("testTemplate2")));

        ASSERT_FALSE(g_entity_mgr->has(eid1, ECS_HASH("testTemplate3")));
        ASSERT_FALSE(g_entity_mgr->has(eid2, ECS_HASH("testTemplate3")));
        ASSERT_FALSE(g_entity_mgr->has(eid3, ECS_HASH("testTemplate3")));
        ASSERT_FALSE(g_entity_mgr->has(eid4, ECS_HASH("testTemplate3")));
        ASSERT_TRUE(g_entity_mgr->has(eid5, ECS_HASH("testTemplate3")));
        ASSERT_TRUE(g_entity_mgr->has(eid6, ECS_HASH("testTemplate3")));
        ASSERT_FALSE(g_entity_mgr->has(eid7, ECS_HASH("testTemplate3")));
        ASSERT_FALSE(g_entity_mgr->has(eid8, ECS_HASH("testTemplate3")));
        ASSERT_TRUE(g_entity_mgr->has(eid9, ECS_HASH("testTemplate3")));

        ASSERT_FALSE(g_entity_mgr->has(eid1, ECS_HASH("test_int")));
        ASSERT_FALSE(g_entity_mgr->has(eid2, ECS_HASH("test_int")));
        ASSERT_TRUE(g_entity_mgr->has(eid3, ECS_HASH("test_int")));
        ASSERT_TRUE(g_entity_mgr->has(eid4, ECS_HASH("test_int")));
        ASSERT_TRUE(g_entity_mgr->has(eid5, ECS_HASH("test_int")));
        ASSERT_TRUE(g_entity_mgr->has(eid6, ECS_HASH("test_int")));
        ASSERT_FALSE(g_entity_mgr->has(eid7, ECS_HASH("test_int")));
        ASSERT_TRUE(g_entity_mgr->has(eid8, ECS_HASH("test_int")));
        ASSERT_TRUE(g_entity_mgr->has(eid9, ECS_HASH("test_int")));

        ASSERT_FALSE(g_entity_mgr->has(eid1, ECS_HASH("test_struct")));
        ASSERT_FALSE(g_entity_mgr->has(eid2, ECS_HASH("test_struct")));
        ASSERT_FALSE(g_entity_mgr->has(eid3, ECS_HASH("test_struct")));
        ASSERT_FALSE(g_entity_mgr->has(eid4, ECS_HASH("test_struct")));
        ASSERT_TRUE(g_entity_mgr->has(eid5, ECS_HASH("test_struct")));
        ASSERT_TRUE(g_entity_mgr->has(eid6, ECS_HASH("test_struct")));
        ASSERT_FALSE(g_entity_mgr->has(eid7, ECS_HASH("test_struct")));
        ASSERT_FALSE(g_entity_mgr->has(eid8, ECS_HASH("test_struct")));
        ASSERT_TRUE(g_entity_mgr->has(eid9, ECS_HASH("test_struct")));

        ASSERT_EQ(g_entity_mgr->getNullable<int>(eid1, ECS_HASH("test_int")), nullptr);
        ASSERT_EQ(g_entity_mgr->getNullable<TestTemplateDeclarationStruct>(eid1, ECS_HASH("test_struct")), nullptr);

        ASSERT_EQ(g_entity_mgr->getNullable<int>(eid2, ECS_HASH("test_int")), nullptr);
        ASSERT_EQ(g_entity_mgr->getNullable<TestTemplateDeclarationStruct>(eid2, ECS_HASH("test_struct")), nullptr);

        ASSERT_NE(g_entity_mgr->getNullable<int>(eid3, ECS_HASH("test_int")), nullptr);
        ASSERT_NE(g_entity_mgr->getNullable<int>(eid4, ECS_HASH("test_int")), nullptr);
        ASSERT_NE(g_entity_mgr->getNullable<int>(eid5, ECS_HASH("test_int")), nullptr);
        ASSERT_NE(g_entity_mgr->getNullable<int>(eid6, ECS_HASH("test_int")), nullptr);

        ASSERT_NE(g_entity_mgr->getNullable<TestTemplateDeclarationStruct>(eid5, ECS_HASH("test_struct")), nullptr);
        ASSERT_NE(g_entity_mgr->getNullable<TestTemplateDeclarationStruct>(eid6, ECS_HASH("test_struct")), nullptr);


        ASSERT_EQ(call_t1(), 3);
        ASSERT_EQ(call_t2(), 3);
        ASSERT_EQ(call_t3(), 3);

        ecs::EntityId eid11;

        {
            TestTemplate3 eid10;
            ASSERT_TRUE(g_entity_mgr->doesEntityExist(eid10));
            eid11 = eid10;
        }
        g_entity_mgr->tick(true);
        ASSERT_FALSE(g_entity_mgr->doesEntityExist(eid11));

    }
}  // namespace nau::test
