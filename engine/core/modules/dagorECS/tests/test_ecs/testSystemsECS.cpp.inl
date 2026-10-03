// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "test_ecs_common.h"

using namespace nau::math;

struct DebugRectangleComponent
{
    uint32_t tex_id = 0;
};

ECS_DECLARE_RELOCATABLE_TYPE(DebugRectangleComponent);


struct DebugRectangleComponentConstruct final : public DebugRectangleComponent
{
    DebugRectangleComponentConstruct(const DebugRectangleComponentConstruct&) = delete; DebugRectangleComponentConstruct& operator=(const DebugRectangleComponentConstruct&) = delete;;
    DebugRectangleComponentConstruct() = default;

    static void requestResources(const char *, const ecs::resource_request_cb_t &) {}

    DebugRectangleComponentConstruct(const ecs::EntityManager &mgr, ecs::EntityId eid)
    {

    }

    ~DebugRectangleComponentConstruct()
    {
    }
};

static void codegen_update_es(
    const ecs::UpdateStageInfoRenderDebug& update,
    ecs::EntityId eid,
    TestStructureComponent& test_structure,
    bool& test_bool,
    IVector2& test_ip2,
    IVector4 test_color = IVector4(255, 255, 255, 255),
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

static void codegen_event_es(
    const MyTestEventAsyncEvent& event,
    ecs::EntityId eid,
    TestStructureComponent& test_structure,
    bool& test_bool,
    IVector2& test_ip2,
    IVector4 test_color = IVector4(255, 255, 255, 255),
    Point2 test_p2 = Point2(0, 0))
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

template<class Callable> inline void codegen_ecs_query(Callable c);

void call_codegen_query()
{
    codegen_ecs_query(
        [&](ecs::EntityId eid,
            TestStructureComponent& test_structure,
            bool& test_bool,
            IVector2& test_ip2,
            IVector4 test_color = IVector4(255, 255, 255, 255),
            Point2 test_p2 = Point2(0, 0))
        {
            test_structure.query_called++;
            if((test_structure.validData.test_bool == test_bool) &&
               (test_structure.validData.test_ip2 == test_ip2) &&
               (test_structure.validData.test_color == test_color) &&
               (test_structure.validData.test_p2 == test_p2))
            {
                test_structure.query_valid++;
            }
        });
}