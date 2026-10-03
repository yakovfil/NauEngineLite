// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "test_ecs_common.h"

#include <utility>
#include <vector>
using namespace nau::math;

ecs::template_t create_template(ecs::ComponentsMap&& map, ecs::Template::component_set&& tracked, const char* name)
{
    static int tn = 0;
    char buf[64];
    if(name)
    {
        sprintf(buf, "%s", name);
    }
    else
    {
        sprintf(buf, "_t%d", tn++);
    }
    auto res = g_entity_mgr->addTemplate(
        ecs::Template(buf, std::move(map), std::move(tracked), ecs::Template::component_set(), ecs::Template::component_set(), false));
    NAU_ASSERT(res == ecs::TemplateDB::AR_OK);
    return g_entity_mgr->instantiateTemplate(g_entity_mgr->buildTemplateIdByName(buf));
}

void validateEntities(const std::vector<ecs::EntityId>& entities, uint32_t event_called, uint32_t update_called, uint32_t query_called)
{
    for(auto eid : entities)
    {
        TestStructureComponent validData = g_entity_mgr->get<TestStructureComponent>(eid, ECS_HASH("test_structure"));
        EXPECT_EQ(validData.resource_loaded, 1);
        EXPECT_EQ(validData.constructor_called, 1);
        EXPECT_EQ(validData.event_called, event_called * 2);
        EXPECT_EQ(validData.update_called, update_called * 2);
        EXPECT_EQ(validData.event_valid, event_called * 2);
        EXPECT_EQ(validData.update_valid, update_called * 2);
        EXPECT_EQ(validData.query_called, query_called);
        EXPECT_EQ(validData.query_valid, query_called);
    }
}

void updateResourcesState(TestResourceManagerImpl* resourceManager)
{
    g_entity_mgr->tick(1);
    std::vector<ecs::EntityId> loadedEntities = resourceManager->wait_all_tasks();
    g_entity_mgr->tick(1);
    for(auto eid : loadedEntities)
    {
        auto& validData = g_entity_mgr->getRW<TestStructureComponent>(eid, ECS_HASH("test_structure"));
        validData.resource_loaded++;
    }
}

ECS_REGISTER_EVENT(MyTestEventAsyncEvent)

ECS_REGISTER_MANAGED_TYPE(TestStructureComponent, nullptr, typename ecs::CreatorSelector<TestStructureComponent ECS_COMMA TestStructureComponentConstruct>::type);

ECS_AUTO_REGISTER_COMPONENT_BASE(ecs::string, "test_string", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(bool, "test_bool", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(IVector4, "test_color", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(IVector2, "test_ip2", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(Point2, "test_p2", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(TestStructureComponent, "test_structure", nullptr, 0, "?test_string", "?test_color", "?test_bool");
ECS_DEF_PULL_VAR(test_structure);
