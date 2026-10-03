// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <nau/rtti/rtti_impl.h>

#include <chrono>
#include <utility>
#include <vector>

#include "daECS/core/componentTypes.h"
#include "daECS/core/coreEvents.h"
#include "daECS/core/dataComponent.h"
#include "daECS/core/entityManager.h"
#include "daECS/core/entitySystem.h"
#include "daECS/core/internal/performQuery.h"
#include "daECS/core/sharedComponent.h"
#include "daECS/core/updateStage.h"
#include "dag_perfTimer.h"
#include "nau/async/task.h"
#include "nau/async/thread_pool_executor.h"
#include "nau/async/work_queue.h"
#include "nau/utils/cancellation.h"
#include "nau/utils/span.h"
#include "test_ecs_common.h"

using namespace nau::math;

static_assert(ecs::TypeCopyConstructible<TestStructureComponent>::value);


static void test_update_es(
    const ecs::UpdateStageInfoRenderDebug& update,
    ecs::EntityId eid,
    TestStructureComponent& test_structure,
    bool& test_bool,
    IVector2& test_ip2,
    IVector4 test_color = {255, 255, 255, 255},
    Point2 test_p2 = Point2(0, 0))
{
    test_structure.update_called++;
    if((test_structure.validData.test_bool == test_bool) &&
       (test_structure.validData.test_ip2 == test_ip2) &&
       (test_structure.validData.test_color == test_color) &&
       (test_structure.validData.test_p2 == test_p2))
    {
        test_structure.update_valid++;
    }
}

static void test_event_es(
    const MyTestEventAsyncEvent& event,
    ecs::EntityId eid,
    TestStructureComponent& test_structure,
    bool& test_bool,
    IVector2& test_ip2,
    IVector4 test_color = {255, 255, 255, 255},
    Point2 test_p2 = {0, 0})
{
    test_structure.event_called++;
    if((test_structure.validData.test_bool == test_bool) &&
       (test_structure.validData.test_ip2 == test_ip2) &&
       (test_structure.validData.test_color == test_color) &&
       (test_structure.validData.test_p2 == test_p2))
    {
        test_structure.event_valid++;
    }
}

static constexpr ecs::ComponentDesc test_update_es_comps[] =
    {
  //  start of 3 rw components at [0]
        {ECS_HASH("test_structure"), ecs::ComponentTypeInfo<TestStructureComponent>()},
        {ECS_HASH("test_bool"), ecs::ComponentTypeInfo<bool>()},
        {ECS_HASH("test_ip2"), ecs::ComponentTypeInfo<IVector2>()},
 //  start of 3 ro components at [3]
        {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
        {ECS_HASH("test_color"), ecs::ComponentTypeInfo<IVector4>(), ecs::CDF_OPTIONAL},
        {ECS_HASH("test_p2"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL}
};
static void test_update_es_all(const ecs::UpdateStageInfo& __restrict info, const ecs::QueryView& __restrict components)
{
    auto comp = components.begin(), compE = components.end();
    NAU_ASSERT(comp != compE);
    do
        test_update_es(*info.cast<ecs::UpdateStageInfoRenderDebug>(),
                                 ECS_RO_COMP(test_update_es_comps, "eid", ecs::EntityId),
                                 ECS_RW_COMP(test_update_es_comps, "test_structure", TestStructureComponent),
                                 ECS_RW_COMP(test_update_es_comps, "test_bool", bool),
                                 ECS_RW_COMP(test_update_es_comps, "test_ip2", IVector2),
                                 ECS_RO_COMP_OR(test_update_es_comps, "test_color", IVector4(IVector4{255, 255, 255, 255})),
                                 ECS_RO_COMP_OR(test_update_es_comps, "test_p2", Point2(Point2{0, 0})));
    while(++comp != compE);
}
static ecs::EntitySystemDesc test_update_es_es_desc(
    "test_update_es",
    "D:/dagorengineCmake/prog/gameLibs/ecs/render/debugRectangleES.cpp.inl",
    ecs::EntitySystemOps(test_update_es_all),
    make_span(test_update_es_comps + 0, 3) /*rw*/,
    make_span(test_update_es_comps + 3, 3) /*ro*/,
    empty_span(),
    empty_span(),
    ecs::EventSetBuilder<>::build(),
    (1 << ecs::UpdateStageInfoRenderDebug::STAGE));
static constexpr ecs::ComponentDesc test_event_es_comps[] =
    {
  //  start of 3 rw components at [0]
        {ECS_HASH("test_structure"), ecs::ComponentTypeInfo<TestStructureComponent>()},
        {ECS_HASH("test_bool"), ecs::ComponentTypeInfo<bool>()},
        {ECS_HASH("test_ip2"), ecs::ComponentTypeInfo<IVector2>()},
 //  start of 3 ro components at [3]
        {ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
        {ECS_HASH("test_color"), ecs::ComponentTypeInfo<IVector4>(), ecs::CDF_OPTIONAL},
        {ECS_HASH("test_p2"), ecs::ComponentTypeInfo<Point2>(), ecs::CDF_OPTIONAL}
};
static void test_event_es_all_events(const ecs::Event& __restrict evt, const ecs::QueryView& __restrict components)
{
    NAU_FAST_ASSERT(evt.is<MyTestEventAsyncEvent>());
    auto comp = components.begin(), compE = components.end();
    NAU_ASSERT(comp != compE);
    do
        test_event_es(static_cast<const MyTestEventAsyncEvent&>(evt),
                            ECS_RO_COMP(test_event_es_comps, "eid", ecs::EntityId),
                            ECS_RW_COMP(test_event_es_comps, "test_structure", TestStructureComponent)
                                , ECS_RW_COMP(test_event_es_comps, "test_bool", bool),
                            ECS_RW_COMP(test_event_es_comps, "test_ip2", IVector2),
                            ECS_RO_COMP_OR(test_event_es_comps, "test_color", IVector4(IVector4{255, 255, 255, 255})),
                            ECS_RO_COMP_OR(test_event_es_comps, "test_p2", Point2(Point2{0, 0}))
                            );
    while(++comp != compE);
}
static ecs::EntitySystemDesc test_event_es_es_desc(
    "test_event_es",
    "D:/dagorengineCmake/prog/gameLibs/ecs/render/debugRectangleES.cpp.inl",
    ecs::EntitySystemOps(nullptr, test_event_es_all_events),
    make_span(test_event_es_comps + 0, 3) /*rw*/,
    make_span(test_event_es_comps + 3, 3) /*ro*/,
    empty_span(),
    empty_span(),
    ecs::EventSetBuilder<MyTestEventAsyncEvent>::build(),
    0);

namespace nau::test
{
    TEST_F(TestDagorECS, Systems)
    {
        // TestStructureComponent &test_structure,
        // bool& test_bool,
        // IVector2& test_ip2,
        // IVector4 test_color = {255, 255, 255, 255},
        // Point2 test_p2 = {0, 0})
        ecs::template_t templ;
        {
            ecs::ComponentsMap map;
            map[ECS_HASH("test_structure")] = TestStructureComponent{};
            map[ECS_HASH("test_bool")] = false;
            map[ECS_HASH("test_ip2")] = IVector2(0, 0);
            map[ECS_HASH("test_color")] = IVector4{};
            map[ECS_HASH("test_p2")] = Point2();
            // map[ECS_HASH("int_component2")] = 10;
            templ = create_template(std::move(map), {}, "theTemplate");
            ecs::ComponentsInitializer map2;
        }
        std::vector<ecs::EntityId> eid;
        for(int j = 0; j < CREATE_RUNS; ++j)
        {
            TestData validData;
            validData.test_bool = j % 2;
            validData.test_color = {j, j, j, j};
            validData.test_ip2 = {j, j};
            validData.test_p2 = {float(j) + 0.5, float(j) + 0.5};
            ecs::ComponentsMap map;
            map[ECS_HASH("test_structure")] = TestStructureComponent{validData};
            //map[ECS_HASH("test_bool")] = validData.test_bool;
            //map[ECS_HASH("test_ip2")] = validData.test_ip2;
            //map[ECS_HASH("test_color")] = validData.test_color;
            //map[ECS_HASH("test_p2")] = validData.test_p2;
            ecs::ComponentsInitializer attrs;
            //ECS_INIT(attrs, test_structure, TestStructureComponent{validData});
            ECS_INIT(attrs, "test_bool", validData.test_bool);
            ECS_INIT(attrs, "test_ip2",  validData.test_ip2);
            ECS_INIT(attrs, "test_color",validData.test_color);
            ECS_INIT(attrs, "test_p2",   validData.test_p2);
            // init[ECS_HASH("int_variable")] = i;
            // if (i%2 == 0)
            //   init[ECS_HASH("vel")] = Point3(0,0,0);
            eid.push_back(g_entity_mgr->createEntitySync("theTemplate", std::move(attrs), std::move(map)));
        }
        updateResourcesState(resourceManager);
        validateEntities(eid, 0, 0);

        {
            g_entity_mgr->setConstrainedMTMode(true);

            for(int j = CREATE_RUNS; j < CREATE_RUNS + CREATE_RUNS; ++j)
            {
                TestData validData;
                validData.test_bool = j % 2;
                validData.test_color = {j, j, j, j};
                validData.test_ip2 = {j, j};
                validData.test_p2 = {float(j) + 0.5, float(j) + 0.5};
                ecs::ComponentsMap map;
                map[ECS_HASH("test_structure")] = TestStructureComponent{validData};
                ecs::ComponentsInitializer attrs;
                //ECS_INIT(attrs, test_structure, TestStructureComponent{validData});
                ECS_INIT(attrs, "test_bool", validData.test_bool);
                ECS_INIT(attrs, "test_ip2",  validData.test_ip2);
                ECS_INIT(attrs, "test_color",validData.test_color);
                ECS_INIT(attrs, "test_p2",   validData.test_p2);
                // init[ECS_HASH("int_variable")] = i;
                // if (i%2 == 0)
                //   init[ECS_HASH("vel")] = Point3(0,0,0);
                eid.push_back(g_entity_mgr->createEntityAsync("theTemplate", std::move(attrs), std::move(map)));
            }
            g_entity_mgr->setConstrainedMTMode(false);

            updateResourcesState(resourceManager);
            validateEntities(eid, 0, 0);
        }
        uint32_t event_called = 0;
        uint32_t update_called = 0;
        uint32_t query_called = 0;

        g_entity_mgr->broadcastEventImmediate(MyTestEventAsyncEvent{});
        event_called++;
        validateEntities(eid, event_called, update_called);
        g_entity_mgr->broadcastEventImmediate(MyTestEventAsyncEvent{});
        event_called++;
        g_entity_mgr->broadcastEventImmediate(MyTestEventAsyncEvent{});
        event_called++;
        g_entity_mgr->broadcastEventImmediate(MyTestEventAsyncEvent{});
        event_called++;
        g_entity_mgr->update(ecs::UpdateStageInfoRenderDebug{});
        update_called++;
        validateEntities(eid, event_called, update_called);

        call_codegen_query();
        query_called++;
        validateEntities(eid, event_called, update_called, query_called);
        call_codegen_query();
        query_called++;
        validateEntities(eid, event_called, update_called, query_called);


        g_entity_mgr->setConstrainedMTMode(true);
        g_entity_mgr->broadcastEvent(MyTestEventAsyncEvent{});
        g_entity_mgr->broadcastEvent(MyTestEventAsyncEvent{});
        g_entity_mgr->broadcastEvent(MyTestEventAsyncEvent{});
        validateEntities(eid, event_called, update_called, query_called);
        g_entity_mgr->setConstrainedMTMode(false);
        g_entity_mgr->tick(true);
        event_called++;
        event_called++;
        event_called++;
        validateEntities(eid, event_called, update_called, query_called);

    }
}  // namespace nau::test
