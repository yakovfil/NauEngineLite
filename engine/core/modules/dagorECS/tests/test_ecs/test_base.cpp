// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <nau/rtti/rtti_impl.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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
#include "nau/app/application_services.h"
#include "nau/async/task.h"
#include "nau/async/thread_pool_executor.h"
#include "nau/async/work_queue.h"
#include "nau/utils/cancellation.h"

enum : uint32_t
{
    TESTS = 5000,
    ECS_RUNS = 10,
    CMP_RUNS = 40,
    CREATE_RUNS = 10,
    EID_QUERY_RUNS = 10,
    Q_CACHE_CNT = 5,
    Q_CNT = 40
};

using namespace std::literals::chrono_literals;
using namespace nau::math;

#define NAU_CORE_DEBUG_LF ECS_LOG

using vec4f = Vector4;
using TMatrix = Matrix4;

ECS_AUTO_REGISTER_COMPONENT(TMatrix, "transform", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(Point3, "pos", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(Point3, "pos$copy", nullptr, 0, "pos");
ECS_AUTO_REGISTER_COMPONENT(Vector3, "vel", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(vec4f, "pos_vec", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(vec4f, "vel_vec", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(int, "int_variable", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::Tag, "tag_sample", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT_DEPS(float, "float_component", nullptr, 0, "int_variable");
ECS_AUTO_REGISTER_COMPONENT_DEPS(int, "int_component2", nullptr, 0, "int_variable");
ECS_AUTO_REGISTER_COMPONENT(ecs::Object, "object", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::string, "str_test", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(ecs::SharedComponent<ecs::string>, "shared_str", nullptr, 0);

namespace ecs
{
    static int common_job_mgr_id = -1;
    struct LoadGameResJob
    {
        ecs::gameres_list_t resnm;
        std::vector<EntityId> entities;
        ::nau::async::Task<> doJob()
        {
            co_await nau::async::Executor::getDefault();
            NAU_CORE_DEBUG_LF("doJob");
            co_await 100ms;
            NAU_CORE_DEBUG_LF("jobDone");
        }
        virtual void releaseJob()
        {
            if(g_entity_mgr && !entities.empty())  // in case if this callback is called after destruction of EntityManager
                g_entity_mgr->onEntitiesLoaded(entities, true);
            delete this;
        }
        virtual ~LoadGameResJob() = default;
    };

    struct TestResourceManagerImpl : public IECSResourceManager
    {
        NAU_RTTI_CLASS(TestResourceManagerImpl, IECSResourceManager)

        bool load_gameres_list(const ecs::gameres_list_t&) override
        {
            return true;
        }
        size_t filter_out_loaded_gameres(ecs::gameres_list_t& list) override
        {
            list.clear();
            return 0;
        }
        std::vector<std::unique_ptr<LoadGameResJob>> jobs;
        std::vector<nau::async::Task<>> jobTasks;
        void async_load_gameres_list(std::vector<ecs::EntityId>&& eids, ecs::gameres_list_t&& nms) override
        {
            NAU_UNUSED(nms);
            for(auto& n : nms)
                NAU_CORE_DEBUG_LF("place_gameres_request <{}>", n.first.c_str());
            // g_entity_mgr->onEntitiesLoaded(eids);
            auto& job = jobs.emplace_back(std::make_unique<LoadGameResJob>());
            job->entities = std::move(eids);
            job->resnm = std::move(nms);
            // int jobMgr = get_common_loading_job_mgr();
            //  if (jobMgr >= 0
            // G_VERIFY(cpujobs::add_job(jobMgr, job));
            jobTasks.emplace_back(job->doJob());
        }

        void wait_all_tasks()
        {
            nau::async::Task<bool> task = nau::async::whenAll(jobTasks, nau::Expiration::never());
            nau::async::wait(task);
        }
        ~TestResourceManagerImpl()
        {
            wait_all_tasks();
        }
    };

};  // namespace ecs

static int refcnt = 0;
struct SampleComponent
{
    int a = 2;
    int b = 1;
    SampleComponent& operator=(const SampleComponent& v)
    {
        a = v.a;
        b = v.b;
        NAU_CORE_DEBUG_LF("copy =");
        return *this;
    }
    SampleComponent& operator=(SampleComponent&& v)
    {
        a = v.a;
        b = v.b;
        NAU_CORE_DEBUG_LF("move =");
        return *this;
    }

    SampleComponent(const SampleComponent& v) :
        a(v.a),
        b(v.b)
    {
        NAU_CORE_DEBUG_LF("copy constr {}", ++refcnt);
    }
    SampleComponent(SampleComponent&& v) :
        a(v.a),
        b(v.b)
    {
        NAU_CORE_DEBUG_LF("move constr {}", refcnt);
    }
    SampleComponent()
    {
        NAU_CORE_DEBUG_LF("def constr {}", ++refcnt);
    }
    static void requestResources(const char* compname, const ecs::resource_request_cb_t& rcb)
    {
        NAU_CORE_DEBUG_LF("request resource for {}", compname);
        NAU_UNUSED(compname);
        rcb("fake name", 0);
    }
    // bool onLoaded() const{return true;}//simple
    bool onLoaded(const ecs::EntityManager& mgr, ecs::EntityId eid) const
    {
        NAU_CORE_DEBUG_LF("onLoaded ({})!", mgr.getOr(eid, ECS_HASH("int_variable"), -1));
        return true;
    }
    ~SampleComponent()
    {
        NAU_CORE_DEBUG_LF("destr {}", --refcnt);
    }
    /*SampleComponent(const ecs::EntityManager &mgr, ecs::EntityId eid, const ecs::ComponentsMap &map)
    {
      a = mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1);
      NAU_CORE_DEBUG_LF("constructor3 {} ({})", mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1),
             map[ECS_HASH("int_variable")].getOr<int>(-1));
    }
    SampleComponent(const ecs::EntityManager &mgr, ecs::EntityId eid)
    {
      a = mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1);
      NAU_CORE_DEBUG_LF("constructor2 {}", mgr.getOr<int>(eid, ECS_HASH("int_variable"), -1));
    }
    SampleComponent(const ecs::ComponentsMap &map)
    {
      a = map[ECS_HASH("int_variable")].getOr<int>(-1);
      NAU_CORE_DEBUG_LF("constructor1 {}", a);
    }*/
    /*SampleComponent()
    {
      a=0;
      NAU_CORE_DEBUG_LF("constructor0 {}", a);
    }
    ~SampleComponent() {NAU_CORE_DEBUG_LF("destructor");}*/
};

ECS_DECLARE_RELOCATABLE_TYPE(SampleComponent);
ECS_REGISTER_RELOCATABLE_TYPE(SampleComponent, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SampleComponent, "sample_component", nullptr, 0);

struct SampleComponent2
{
    int a;
};
ECS_DECLARE_RELOCATABLE_TYPE(SampleComponent2);
ECS_REGISTER_RELOCATABLE_TYPE(SampleComponent2, nullptr);
ECS_AUTO_REGISTER_COMPONENT(SampleComponent2, "sample_component2", nullptr, 0);

volatile int cache0 = 0;

void prune_cache()
{
    static std::vector<int> memory;
    if(!memory.size())
        memory.resize(4 << 20, 1);
    for(auto i : memory)
        cache0 += i;
}

void testAllocator()
{
    StackAllocator<8> allocator;
    auto testEmpty = [&](int cnt, int blocks_cnt, uint32_t size)
    {
        size_t mem = allocator.calcMemAllocated();
        NAU_UNUSED(mem);
        for(int i = 0; i < cnt; ++i)
        {
            std::vector<uint8_t*> blocks(blocks_cnt);
            for(auto& b : blocks)
                b = allocator.allocate(size);
            NAU_ASSERT(!allocator.chunks.empty());
            for(auto bi = blocks.rbegin(), e = blocks.rend(); bi != e; ++bi)
                allocator.deallocate(*bi, size);
        }
        NAU_ASSERT(allocator.calcMemAllocated() == mem);
    };
    NAU_ASSERT(allocator.calcMemAllocated() == 0);
    testEmpty(10, 100, 9);
    testEmpty(3, 100, 16);
    testEmpty(3, 100, 24);
    NAU_CORE_DEBUG_LF("allocator tested");
}

void testObject()
{
    ECS_LOG("object");
    ecs::Object object;
    object.insert(ECS_HASH("1")) = 11;
    ECS_LOG("current {}\n", object[ECS_HASH("1")].get<int>());
    object.insert(ECS_HASH("1")) = 2;
    ECS_LOG("current {}\n", object[ECS_HASH("1")].get<int>());
    object.insert(ECS_HASH("2")) = 1.0f;
    object.insert(ECS_HASH("obj")) = ecs::Object();
    object.insert(ECS_HASH("obj")).getRW<ecs::Object>().insert(ECS_HASH("1")) = 13;
    ecs::Object& child = object.insert(ECS_HASH("obj")).getRW<ecs::Object>();
    ECS_LOG("current child.1 {}\n", child[ECS_HASH("1")].get<int>());
    child.insert(ECS_HASH("1")) = 14;
    ECS_LOG("current child.1 {}\n", child[ECS_HASH("1")].get<int>());
    ecs::Object object2 = object;
    ECS_LOG("compare object == object2 = {}\n", object2 == object);
    object.insert(ECS_HASH("obj")).getRW<ecs::Object>().insert(ECS_HASH("obj")) = ecs::Object();
    ECS_LOG("compare object == object2 = {}\n", object2 == object);
}

void testArray()
{
    ECS_LOG("array");
    ecs::Array object;
    object.push_back(11);
    ECS_LOG("current {}\n", object[0].get<int>());
    object[0] = 2;
    ECS_LOG("current {}\n", object[0].get<int>());
    object.push_back(1.0f);
    object.push_back(ecs::Object());
    object[2].getRW<ecs::Object>().insert(ECS_HASH("1")) = 13;
    ecs::Object& child = object[2].getRW<ecs::Object>();
    ECS_LOG("current child.1 {}\n", child[ECS_HASH("1")].get<int>());
    child.insert(ECS_HASH("1")) = 14;
    ECS_LOG("current child.1 {}\n", child[ECS_HASH("1")].get<int>());
}

static constexpr ecs::ComponentDesc kinematics_comps[] = {
    {         ECS_HASH("pos"),   ecs::ComponentTypeInfo<Point3>()}, // rw
    {         ECS_HASH("vel"),  ecs::ComponentTypeInfo<Vector3>()}, // ro
    {ECS_HASH("int_variable"),      ecs::ComponentTypeInfo<int>()}, // rq
    {  ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()}  // no
};
static constexpr ecs::ComponentDesc kinematics_events_comps[] = {
    {ECS_HASH("pos"), ecs::ComponentTypeInfo<Point3>()}, // ro
};

static __forceinline void kinematics_es(const ecs::UpdateStageInfoAct& info, Point3& p, const Vector3& v)
{
    p += v * info.dt;
}

static void kinematics_es_all(const ecs::UpdateStageInfo& info, const ecs::QueryView& components)
{
    const ecs::UpdateStageInfoAct& act = (const ecs::UpdateStageInfoAct&)info;
    auto* __restrict pos = ECS_QUERY_COMP_RW_PTR(Point3, kinematics_comps, "pos");
    auto* __restrict posE = pos + components.end();
    pos += components.begin();
    auto* __restrict vel = ECS_QUERY_COMP_RO_PTR(Vector3, kinematics_comps, "vel") + components.begin();
    do
    {
        kinematics_es(act, ecs::getRef(pos), ecs::getRef(vel));  // somehow getRef is slower, codegen just * or ** depending on type
        // kinematics_es(act, *pos, *vel);
        pos++;
        vel++;
    } while(pos < posE);
}

Point3 ppp(0, 0, 0);

static void kinematics_es_event_all(const ecs::Event&, const ecs::QueryView& components)
{
    auto* __restrict pos = ECS_QUERY_COMP_RO_PTR(Point3, kinematics_events_comps, "pos") + components.begin();
    for(uint32_t i = 0, ei = components.getEntitiesCount(); i < ei; ++i)
    {
        ppp = ppp + Vector3(ecs::getRef(pos));
        pos++;
    }
}

static ecs::EntitySystemDesc kinematics_es_desc("kinematics_es", ecs::EntitySystemOps(kinematics_es_all, nullptr), std::span(kinematics_comps + 0, 1), std::span(kinematics_comps + 1, 1), std::span(kinematics_comps + 2, 1), std::span(kinematics_comps + 3, 1),
                                                ecs::EventSetBuilder<>::build(),  // ecs::EventComponentChanged
                                                (1 << ecs::UpdateStageInfoAct::STAGE));

static ecs::EntitySystemDesc kinematics_events_es_desc("kinematics_events_es", ecs::EntitySystemOps(nullptr, kinematics_es_event_all), nau::ConstSpan<ecs::ComponentDesc>(),
                                                       // std::span(kinematics_events_comps_rw),
                                                       make_span(kinematics_events_comps),
                                                       nau::ConstSpan<ecs::ComponentDesc>(),
                                                       nau::ConstSpan<ecs::ComponentDesc>(),
                                                       ecs::EventSetBuilder<ecs::EventComponentChanged>::build(),  //
                                                       0);

static constexpr ecs::ComponentDesc object_events_comps[] = {
    {ECS_HASH("object"), ecs::ComponentTypeInfo<ecs::Object>()}  // ro
};

static void object_es_event_all(const ecs::Event&, const ecs::QueryView& components)
{
    auto* __restrict pos = ECS_QUERY_COMP_RO_PTR(ecs::Object, object_events_comps, "object");
    for(uint32_t i = 0, ei = components.getEntitiesCount(); i < ei; ++i)
    {
        pos++;
    }
    NAU_CORE_DEBUG_LF("changed {}", components.getEntitiesCount());
    // NAU_CORE_DEBUG_LF("changed = {}", evt.cast<ecs::EventComponentChanged>()->get<0>());
}

static ecs::EntitySystemDesc object_events_es_desc("object_events_es", ecs::EntitySystemOps(nullptr, object_es_event_all), nau::ConstSpan<ecs::ComponentDesc>(), std::span(object_events_comps), nau::ConstSpan<ecs::ComponentDesc>(), nau::ConstSpan<ecs::ComponentDesc>(),
                                                   ecs::EventSetBuilder<ecs::EventComponentChanged>::build(),  //
                                                   0);

struct TestEntity
{
    TMatrix transform = {TMatrix::identity()};
    int iv = 10, ic2 = 10;
    Point3 p = {1, 0, 0};
    TMatrix d[9] = {TMatrix::identity()};
    Vector3 v = {1, 0, 0};
    int ivCopy = 10;
    virtual void update(float dt)
    {
        p += dt * v;
    }
    TestEntity() = default;
    TestEntity(int i) :
        iv(i),
        ivCopy(i)
    {
    }
    virtual ~TestEntity() = default;
};

static bool check_string_relocatable()
{
    ecs::string sh = "a";
    ecs::string ln = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    char buf[sizeof(ecs::string)];
    memcpy(buf, &ln, sizeof(ecs::string));
    NAU_ASSERT_RETURN(ln == *(const ecs::string*)buf, false);
    memcpy(buf, &sh, sizeof(ecs::string));
    NAU_ASSERT_RETURN(sh == *(const ecs::string*)buf, false);
    return true;
}

static ecs::LTComponentList int_variable_component(ECS_HASH("int_variable"), ECS_HASH("int").hash, __FILE__, "DagorWinMain", __LINE__);
static ecs::LTComponentList int_component2_component(ECS_HASH("int_component2"), ECS_HASH("int").hash, __FILE__, "DagorWinMain", __LINE__);

NAU_NOINLINE
static void compare_gets()
{
    static constexpr ecs::ComponentDesc comps[] = {
        // ro
        {           ECS_HASH("eid"), ecs::ComponentTypeInfo<ecs::EntityId>()},
        // rq
        {  ECS_HASH("int_variable"),           ecs::ComponentTypeInfo<int>()},
        {ECS_HASH("int_component2"),           ecs::ComponentTypeInfo<int>()},
        // no
        {    ECS_HASH("tag_sample"),      ecs::ComponentTypeInfo<ecs::Tag>()}
    };
    ecs::NamedQueryDesc desc{
        "q_compare_gets",
        empty_span(),
        std::span(comps + 0, 1),
        std::span(comps + 1, 2),
        std::span(comps + 3, 1),
    };
    static ecs::QueryId persistentQuery = g_entity_mgr->createQuery(desc);

    static constexpr ecs::ComponentDesc calc_comps[] = {
        {  ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()},
        {ECS_HASH("int_component2"), ecs::ComponentTypeInfo<int>()},
        //{ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()}
    };
    ecs::NamedQueryDesc descCalc{
        "q1C",
        empty_span(),
        std::span(calc_comps),
        empty_span(),
        empty_span(),
    };
    static ecs::QueryId calc_query_id = g_entity_mgr->createQuery(descCalc);

    int int_component_calc = 0;
    int64_t reft;
    std::vector<ecs::EntityId> eids;
    ecs::perform_query(g_entity_mgr, persistentQuery, [&](const ecs::QueryView& qv)
                       {
                           for(auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
                               eids.push_back(qv.getComponentRO<ecs::EntityId>(ECS_QUERY_COMP_RO_INDEX(comps, "eid"), it));
                       });
    uint32_t time = ~0u;
    for(int i = 0; i < EID_QUERY_RUNS; ++i)
    {
        reft = profile_ref_ticks();
        int_component_calc = 0;
        for(auto eid : eids)
        {
            perform_query(g_entity_mgr, eid,  // qv.getComponentRO<ecs::EntityId>(ECS_QUERY_COMP_RO_INDEX(comps, "eid"), it)
                          calc_query_id, [&int_component_calc](const ecs::QueryView& qv)
                          {
                              int_component_calc += qv.getComponentRO<int>(0, 0) + qv.getComponentRO<int>(1, 0);
                          });
        }
        uint32_t ctime = profile_time_usec(reft);
        if(time > ctime)
            time = ctime;
    }
    NAU_CORE_DEBUG_LF("single eid get query in {}us, ret= {}", time, int_component_calc);

    time = ~0u;
    for(int i = 0; i < EID_QUERY_RUNS; ++i)
    {
        reft = profile_ref_ticks();
        int_component_calc = 0;
        for(auto eid : eids)
            int_component_calc += *ECS_GET_NULLABLE(int, eid, int_variable) + *ECS_GET_NULLABLE(int, eid, int_component2);
        uint32_t ctime = profile_time_usec(reft);
        if(time > ctime)
            time = ctime;
    }
    NAU_CORE_DEBUG_LF("fast get query in {}us, ret= {}", time, int_component_calc);

    time = ~0u;
    for(int i = 0; i < EID_QUERY_RUNS; ++i)
    {
        reft = profile_ref_ticks();
        int_component_calc = 0;
        for(auto eid : eids)
            int_component_calc += *g_entity_mgr->getNullable<int>(eid, ECS_HASH("int_variable")) +
                                  *g_entity_mgr->getNullable<int>(eid, ECS_HASH("int_component2"));
        uint32_t ctime = profile_time_usec(reft);
        if(time > ctime)
            time = ctime;
    }
    NAU_CORE_DEBUG_LF("get query in {}us, ret= {}", time, int_component_calc);

    time = ~0u;
    for(int i = 0; i < EID_QUERY_RUNS; ++i)
    {
        reft = profile_ref_ticks();
        int_component_calc = 0;
        ecs::perform_query(g_entity_mgr, calc_query_id, [&int_component_calc](const ecs::QueryView& qv)
                           {
                               for(auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
                                   int_component_calc += qv.getComponentRO<int>(0, it) + qv.getComponentRO<int>(1, it);
                           });
        uint32_t ctime = profile_time_usec(reft);
        if(time > ctime)
            time = ctime;
    }
    NAU_CORE_DEBUG_LF("just query in {}us, ret= {}", time, int_component_calc);
}

#include <future>

NAU_NOINLINE
static void contrained_mt_mode_example()
{
    static constexpr ecs::ComponentDesc comps[] = {
        {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>()}
    };
    ecs::NamedQueryDesc descRead{
        "thread1",
        empty_span(),
        std::span(comps),
        empty_span(),
        empty_span(),
    };
    ecs::NamedQueryDesc descWrite{
        "thread1",
        std::span(comps),
        empty_span(),
        empty_span(),
        empty_span(),
    };
    ecs::QueryId read_intQuery = g_entity_mgr->createQuery(descRead);
    ecs::QueryId write_intQuery = g_entity_mgr->createQuery(descWrite);
    NAU_UNUSED(write_intQuery);

    {
        ecs::ComponentsMap map;
        map[ECS_HASH("int_variable")] = 10;
        ecs::Template::component_set tracked;
        tracked.insert(ECS_HASH("int_variable").hash);
        ecs::Template templD("tsanTemplate1", std::move(map), std::move(tracked), ecs::Template::component_set(),
                             ecs::Template::component_set(), false);
        g_entity_mgr->addTemplate(std::move(templD));
    }
    {
        ecs::ComponentsMap map;
        map[ECS_HASH("int_variable")] = 100;
        ecs::Template templD("tsanTemplate2", std::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
                             ecs::Template::component_set(), false);
        g_entity_mgr->addTemplate(std::move(templD));
    }

    {
        ecs::ComponentsMap map;
        map[ECS_HASH("int_variable")] = 100;
        map[ECS_HASH("int_variable2")] = 100;
        ecs::Template templD("tsanTemplate3", ecs::ComponentsMap(map), ecs::Template::component_set(), ecs::Template::component_set(),
                             ecs::Template::component_set(), false);
        g_entity_mgr->addTemplate(std::move(templD));

        map[ECS_HASH("int_variable3")] = 100;
        ecs::Template templD2("tsanTemplate4", std::move(map), ecs::Template::component_set(), ecs::Template::component_set(),
                              ecs::Template::component_set(), false);
        g_entity_mgr->addTemplate(std::move(templD2));
    }

    ecs::EntityId eid1 = g_entity_mgr->createEntitySync("tsanTemplate1");
    g_entity_mgr->setConstrainedMTMode(true);
    auto create = [&]()
    {
        int64_t reft = profile_ref_ticks();
        do
        {
            static constexpr int CNT = 100;
            ecs::EntityId eid3[CNT];

            for(int i = 0; i < CNT; ++i)
            {
                eid3[i] = g_entity_mgr->createEntityAsync("tsanTemplate2");
                g_entity_mgr->reCreateEntityFromAsync(eid3[i], "tsanTemplate3");
                g_entity_mgr->reCreateEntityFromAsync(eid3[i], "tsanTemplate4");
            }

            for(int i = 0; i < CNT; ++i)
                g_entity_mgr->destroyEntityAsync(eid3[i]);
        } while(profile_time_usec(reft) < 100000);  // perform for one second
    };
    auto read = [&]()
    {
        std::this_thread::sleep_for(10ms);
        int64_t reft = profile_ref_ticks();
        int readValues = 0;
        do
        {
            ecs::perform_query(g_entity_mgr, read_intQuery, [&](const ecs::QueryView& qv)
                               {
                                   auto& w = qv.getComponentRO<int>(0, 0);
                                   readValues += w;
                               });
            readValues += g_entity_mgr->get<int>(eid1, ECS_HASH("int_variable"));
            // ECS_LOG("thread {} readValues = {}\n", int(get_current_thread_id()), readValues);
            std::this_thread::sleep_for(1ms);
        } while(profile_time_usec(reft) < 100000);  // perform for one second
        ECS_LOG("thread {} readValues = {}\n", int(std::hash<std::thread::id>{}((std::this_thread::get_id()))), readValues);
    };

    auto future1 = std::async(std::launch::async, read), future2 = std::async(std::launch::async, read),
         future3 = std::async(std::launch::async, create);
    // NAU_UNUSED(write_intQuery);
    while(future1.wait_for(std::chrono::milliseconds(1020)) != std::future_status::ready ||
          future2.wait_for(std::chrono::milliseconds(1020)) != std::future_status::ready ||
          future3.wait_for(std::chrono::milliseconds(1020)) != std::future_status::ready)
    {
        std::this_thread::sleep_for(1ms);
        create();
    }
    ECS_LOG("ended\n");
    g_entity_mgr->setConstrainedMTMode(false);
    g_entity_mgr->destroyEntityAsync(eid1);
    g_entity_mgr->destroyQuery(read_intQuery);
    g_entity_mgr->destroyQuery(write_intQuery);
    g_entity_mgr->tick();
}

ecs::template_t create_template(ecs::ComponentsMap&& map, ecs::Template::component_set&& tracked = ecs::Template::component_set(), std::string* name = nullptr)
{
    static int tn = 0;
    char buf[64];
    sprintf(buf, "_t%d", tn++);
    g_entity_mgr->addTemplate(
        ecs::Template(buf, std::move(map), std::move(tracked), ecs::Template::component_set(), ecs::Template::component_set(), false));
    if(name)
        *name = buf;
    return g_entity_mgr->instantiateTemplate(g_entity_mgr->buildTemplateIdByName(buf));
}
ecs::template_t create_template(ecs::ComponentsMap&& map, const char* track)
{
    ecs::Template::component_set tracked;
    NAU_ASSERT(strchr(track, '^') == 0);
    tracked.insert(ECS_HASH_SLOW(track).hash);
    return create_template(std::move(map), std::move(tracked));
}

void testRecreate()
{
    ecs::template_t templ, templ2;
    std::string templ2Name;
    {
        ecs::ComponentsMap map;
        for(int i = 0; i < 128; ++i)
        {
            nau::string str("bool_var_{}", i);
            map[ECS_HASH_SLOW(str.c_str())] = i ? true : false;
        }
        for(int i = 0; i < 128; ++i)
        {
            nau::string str("int_var_{}", i);
            map[ECS_HASH_SLOW(str.c_str())] = i;
        }
        for(int i = 0; i < 512; ++i)
        {
            nau::string str("int2_var_{}", i);
            map[ECS_HASH_SLOW(str.c_str())] = IVector2(i, i);
        }
        for(int i = 0; i < 512; ++i)
        {
            nau::string str("point3_var_{}", i);
            map[ECS_HASH_SLOW(str.c_str())] = Point3(i, i, i);
        }
        {
            ecs::ComponentsMap map2 = map;
            map2["sample_component2"];
            templ = create_template(std::move(map2));
        }
        map[ECS_HASH("tag_sample")] = ecs::Tag();
        templ2 = create_template(std::move(map), ecs::Template::component_set(), &templ2Name);
    }
    enum
    {
        TESTS = 100,
        RUNS = 200
    };
    std::vector<ecs::EntityId> eid(TESTS);
    uint64_t bestCreate = ~uint64_t(0), bestRecreate = ~uint64_t(0);
    int64_t reft;
    for(int r = 0; r < RUNS; ++r)
    {
        reft = profile_ref_ticks();
        for(auto& i : eid)
            i = g_entity_mgr->createEntitySync(templ);
        bestCreate = min(bestCreate, uint64_t(profile_ref_ticks() - reft));

        g_entity_mgr->tick();
        g_entity_mgr->tick();
        g_entity_mgr->tick();
        g_entity_mgr->tick();

        for(auto& i : eid)
            g_entity_mgr->reCreateEntityFromAsync(i, templ2Name.c_str());
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        bestRecreate = min(bestRecreate, uint64_t(profile_ref_ticks() - reft));

        for(auto& i : eid)
            g_entity_mgr->destroyEntityAsync(i);
        g_entity_mgr->tick();
    }
    NAU_CORE_DEBUG_LF("Create={}us bestRecreate  = {}us", profile_usec_from_ticks_delta(bestCreate), profile_usec_from_ticks_delta(bestRecreate));
};

void testSampleComponent()
{
    ecs::ComponentsMap map;
    // map[ECS_HASH("sample_component")];//ecs::ChildComponent();
    map[ECS_HASH("sample_component")] = SampleComponent();
    map[ECS_HASH("int_variable")] = 13;
    std::string templ2Name;
    ecs::template_t templ2 = create_template(std::move(map), ecs::Template::component_set(), &templ2Name);
    ecs::ComponentsInitializer init;
    init[ECS_HASH("int_variable")] = 27;
    ecs::ComponentsMap map2;
    map2[ECS_HASH("int_variable")] = 17;
    auto eid2 = g_entity_mgr->createEntitySync(templ2, std::move(init), std::move(map2));
    NAU_CORE_DEBUG_LF("getOr {}", g_entity_mgr->getOr(eid2, ECS_HASH("int_variable"), -1));
    while(g_entity_mgr->getNumComponents(eid2) < 0)
    {
        NAU_CORE_DEBUG_LF("wait");
        std::this_thread::sleep_for(2ms);
        // cpujobs::release_done_jobs();
        g_entity_mgr->tick();
    };
    // if (0)
    {
        ecs::ComponentsInitializer init;
        init[ECS_HASH("int_variable")] = 29;
        g_entity_mgr->reCreateEntityFromAsync(eid2, templ2Name.c_str(), ecs::ComponentsInitializer(init));
        // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
        // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
    }
    g_entity_mgr->tick();

    static constexpr ecs::ComponentDesc comps[] = {
        {ECS_HASH("sample_component"), ecs::ComponentTypeInfo<SampleComponent>()},
        {    ECS_HASH("int_variable"),             ecs::ComponentTypeInfo<int>()}
    };
    ecs::NamedQueryDesc desc{
        "q2",
        std::span(comps),
        nau::ConstSpan<ecs::ComponentDesc>(),
        nau::ConstSpan<ecs::ComponentDesc>(),
        nau::ConstSpan<ecs::ComponentDesc>(),
    };
    auto qid = g_entity_mgr->createQuery(desc);
    ecs::perform_query(g_entity_mgr, qid, [](const ecs::QueryView& components)
                       {
                           for(auto it = components.begin(), endIt = components.end(); it != endIt; ++it)
                           {
                               ECS_LOG("a = {}\n", components.getComponentRW<SampleComponent>(ECS_QUERY_COMP_RW_INDEX(comps, "sample_component"), it).a);
                               ECS_LOG("int var = {}\n", components.getComponentRW<int>(ECS_QUERY_COMP_RW_INDEX(comps, "int_variable"), it));
                           }
                       });
    g_entity_mgr->destroyQuery(qid);
    ECS_LOG("a = {}\n", g_entity_mgr->get<SampleComponent>(eid2, ECS_HASH("sample_component")).a);

    ECS_LOG("a = {}\n", g_entity_mgr->getEntityComponentRef(eid2, 0).get<SampleComponent>().a);

    // debug_cp();
    // eid2 = g_entity_mgr->reCreateEntityFromSync(eid2, templ);
    // debug_cp();
    g_entity_mgr->destroyEntityAsync(eid2);
    g_entity_mgr->tick();
    // exit(0);
}

void testSharedComponent()
{
    // test SharedComponent
    ecs::ComponentsMap map;
    // map[ECS_HASH("sample_component")] = ecs::ChildComponent();

    map[ECS_HASH("shared_str")] = ecs::SharedComponent<ecs::string>(ecs::string("test_string"));
    auto templ = create_template(std::move(map));
    auto eid = g_entity_mgr->createEntitySync(templ);
    auto eid2 = g_entity_mgr->createEntitySync(templ);
    // const ecs::string &shared_str = g_entity_mgr->get<SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str"));
    NAU_CORE_DEBUG_LF("get shared_str <{}>", g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str"))->c_str());

    // const cast to change shared component for test
    *const_cast<ecs::string*>(g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str")).get()) =
        "changed_string_to_long_enough_string";
    NAU_CORE_DEBUG_LF("get shared_str <{}>", g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid2, ECS_HASH("shared_str"))->c_str());
    NAU_ASSERT(g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid2, ECS_HASH("shared_str"))->c_str() ==
               g_entity_mgr->get<ecs::SharedComponent<ecs::string>>(eid, ECS_HASH("shared_str"))->c_str());
}

static void profileQuery(ecs::QueryId q)
{
    uint64_t bestQ = ~uint64_t(0), avgQ = 0;
    const int qRuns = 1000;
    for(int i = qRuns; i >= 0; --i)
    {
        uint64_t reft = profile_ref_ticks();
        ecs::perform_query(g_entity_mgr, q, [](const ecs::QueryView&)
                           {
                           });
        const uint64_t c = profile_ref_ticks() - reft;
        avgQ += c;
        bestQ = std::min(bestQ, c);
    }
    NAU_CORE_DEBUG_LF("zero cost query cost best {}ticks {}us, avg = {} ({}us)", bestQ, profile_usec_from_ticks_delta(bestQ), double(avgQ / qRuns),
                      double(profile_usec_from_ticks_delta(avgQ)) / qRuns);
}

void __debug_cp(const char* fn, int ln)
{
    NAU_LOG_INFO({"ecs_test"}, "File: {}. Line: {}.", fn, ln);
};

#define debug_cp() __debug_cp(__FILE__, __LINE__)

void testCreateObjectOfArray()
{
    // test object of array
    ecs::ComponentsMap map;
    // map[ECS_HASH("sample_component")] = ecs::ChildComponent();

    map[ECS_HASH("object")] = ecs::Object();
    map[ECS_HASH("object")].getRW<ecs::Object>().insert(ECS_HASH("int_data")) = 111;
    map[ECS_HASH("object")].getRW<ecs::Object>().insert(ECS_HASH("array_data")) = ecs::Array();
    map[ECS_HASH("object")].getRW<ecs::Object>().insert(ECS_HASH("array_data")).getRW<ecs::Array>().push_back(2.0f);
    map[ECS_HASH("int_variable")] = 13;
    map[ECS_HASH("str_test")] = ecs::string("def_temp");
    ecs::template_t templ = create_template(std::move(map), "object");
    ecs::ComponentsInitializer init;
    init[ECS_HASH("int_variable")] = 27;
    ecs::ComponentsMap map2;
    map2[ECS_HASH("int_variable")] = 17;
    init[ECS_HASH("str_test")] = ecs::string("test");
    auto eid2 = g_entity_mgr->createEntitySync(templ, std::move(init), std::move(map2));
    debug_cp();
    NAU_CORE_DEBUG_LF("string {}", g_entity_mgr->getOr(eid2, ECS_HASH("no"), "def"));
    NAU_CORE_DEBUG_LF("string {}", g_entity_mgr->get<ecs::string>(eid2, ECS_HASH("str_test")).c_str());

    ECS_LOG("object.int_data = {}\n", g_entity_mgr->get<ecs::Object>(eid2, ECS_HASH("object"))[ECS_HASH("int_data")].get<int>());
    ECS_LOG("object.array_data[0] = {}\n",
            g_entity_mgr->get<ecs::Object>(eid2, ECS_HASH("object"))[ECS_HASH("array_data")].get<ecs::Array>()[0].get<float>());
    g_entity_mgr->getRW<ecs::Object>(eid2, ECS_HASH("object")).insert(ECS_HASH("array_data")).getRW<ecs::Array>()[0] = 3.0f;
    ECS_LOG("object.array_data[0] = {}\n",
            g_entity_mgr->get<ecs::Object>(eid2, ECS_HASH("object"))[ECS_HASH("array_data")].get<ecs::Array>()[0].get<float>());
    NAU_CORE_DEBUG_LF("check changed");
    g_entity_mgr->tick();
    NAU_CORE_DEBUG_LF("check changed-");
    g_entity_mgr->destroyEntityAsync(eid2);
    g_entity_mgr->tick();
    // exit(0);
}

namespace nau::test
{
    TEST(DagorECS, Base)
    {
        // static const char *filter[] = {"int_variable"};
        // g_entity_mgr->setFilterTags(std::span(filter));
        auto app = createApplication();
        app->startupOnCurrentThread();
        ASSERT_TRUE(getApplication().isMainThread());
        testAllocator();
        g_entity_mgr.demandInit();
        ecs::setECSResourceManager(std::make_unique<ecs::TestResourceManagerImpl>());
        // testRecreate();//return 0;
        contrained_mt_mode_example();
        {
#define DEBUG_TYPE_INFO(type)                                                                                                      \
    NAU_CORE_DEBUG_LF("std::is_copy_assignable<" #type ">::value = {}", std::is_copy_assignable<type>::value);                     \
    NAU_CORE_DEBUG_LF("std::is_copy_constructible<" #type ">::value = {}", std::is_copy_constructible<type>::value);               \
    NAU_CORE_DEBUG_LF("std::is_move_assignable<" #type ">::value = {}", std::is_move_assignable<type>::value);                     \
    NAU_CORE_DEBUG_LF("std::is_trivially_move_assignable<" #type ">::value = {}", std::is_trivially_move_assignable<type>::value); \
    NAU_CORE_DEBUG_LF("std::is_move_constructible<" #type ">::value = {}", std::is_move_constructible<type>::value);

            DEBUG_TYPE_INFO(ecs::Object);
            DEBUG_TYPE_INFO(ecs::Array);
            DEBUG_TYPE_INFO(ecs::ChildComponent);
            DEBUG_TYPE_INFO(ecs::ChildComponent);
            DEBUG_TYPE_INFO(ecs::ComponentsInitializer);
            DEBUG_TYPE_INFO(ecs::ComponentsMap);
            DEBUG_TYPE_INFO(SampleComponent);
#undef DEBUG_TYPE_INFO
        }
        // testObject();
        // testArray();
        auto fillTemplateComponentMap = [&]()
        {
            ecs::ComponentsMap map;
            map[ECS_HASH("int_variable")] = 10;
            map[ECS_HASH("int_component2")] = 10;
            map[ECS_HASH("pos")] = Point3(1, 0, 0);
            map[ECS_HASH("vel")] = Vector3(1, 0, 0);
            map[ECS_HASH("data0")] = TMatrix::identity();
            map[ECS_HASH("data1")] = TMatrix::identity();
            map[ECS_HASH("data2")] = TMatrix::identity();
            map[ECS_HASH("data3")] = TMatrix::identity();
            map[ECS_HASH("data4")] = TMatrix::identity();
            map[ECS_HASH("data5")] = TMatrix::identity();
            map[ECS_HASH("data6")] = TMatrix::identity();
            map[ECS_HASH("data7")] = TMatrix::identity();
            map[ECS_HASH("data8")] = TMatrix::identity();
            map[ECS_HASH("data9")] = TMatrix::identity();
            // map[ECS_HASH("pos$copy")] = Point3(0,0,0);
            // map[ECS_HASH("float_component")] = ecs::Attribute(10.5f);
            // map[ECS_HASH("int_component2")] = ecs::Attribute(20);
            return map;
        };
        ecs::template_t t1;
        {
            ecs::ComponentsMap templMap = fillTemplateComponentMap();
            ecs::Template::component_set tracked;
            tracked.insert(ECS_HASH("int_component2").hash);
            tracked.insert(ECS_HASH("int_variable").hash);
            ecs::Template templD("theTemplate1", std::move(templMap), std::move(tracked), ecs::Template::component_set(),
                                 ecs::Template::component_set(), false);
            g_entity_mgr->addTemplate(std::move(templD));
            auto eid = g_entity_mgr->createEntitySync("theTemplate1");
            t1 = g_entity_mgr->getEntityTemplateId(eid);
            g_entity_mgr->destroyEntityAsync(eid);

            g_entity_mgr->tick();
        }
        ecs::template_t templ = t1;
        NAU_ASSERT(templ != ecs::INVALID_TEMPLATE_INDEX);
        std::vector<ecs::EntityId> eid(TESTS);
        int64_t reft = profile_ref_ticks();
        uint64_t bestCreate = ~uint64_t(0);
        for(int j = 0; j < CREATE_RUNS; ++j)
        {
            int64_t reft = profile_ref_ticks();
            for(int i = 0; i < TESTS; ++i)
            {
                // eid.data()[i] = g_entity_mgr->createEntitySync(templ);
                ecs::ComponentsInitializer init;
                ECS_INIT(init, "int_variable", i);
                // init[ECS_HASH("int_variable")] = i;
                // if (i%2 == 0)
                //   init[ECS_HASH("vel")] = Point3(0,0,0);
                eid.data()[i] = g_entity_mgr->createEntitySync("theTemplate1", std::move(init));  //"theTemplate1",templ
                                                                                                  // eid.data()[i] = g_entity_mgr->createEntitySync(templ);
                                                                                                  // eid = g_entity_mgr->createEntitySync(templ, ecs::ComponentsMap());
            }
            bestCreate = min(bestCreate, uint64_t(profile_ref_ticks() - reft));
            if(j != CREATE_RUNS - 1)
            {
                for(int i = TESTS - 1; i >= 0; --i)
                    g_entity_mgr->destroyEntityAsync(eid.data()[i]);
                g_entity_mgr->tick();
            }
        }
        NAU_CORE_DEBUG_LF("total create time = {} us", profile_usec_from_ticks_delta(bestCreate));
        // for (int i = 0; i < 500; ++i)//to move to one chunk, simulate relaxation during many frames
        //   g_entity_mgr->tick();
        {
            int64_t reft = profile_ref_ticks();
            std::vector<TestEntity> tests(TESTS);
            for(int i = 0; i < TESTS; ++i)
                tests[i].iv = tests[i].ivCopy = i;
            NAU_CORE_DEBUG_LF("best possible (single alloc, init iv) create time = {} us", profile_time_usec(reft));
            {
                int64_t reft = profile_ref_ticks();
                std::vector<TestEntity> tests;
                for(int i = 0; i < TESTS; ++i)
                    tests.emplace_back(TestEntity(i));
                NAU_CORE_DEBUG_LF("best possible grow create time = {} us", profile_time_usec(reft));
            }
            reft = profile_ref_ticks();
            for(int i = 0; i < TESTS; ++i)
            {
                if (tests.size() > 1)
                    tests.front() = std::move(tests.back());
                tests.pop_back();
            }
            NAU_CORE_DEBUG_LF("best (erase_unsorted) possible reverse destroy time = {} us", profile_time_usec(reft));
        }
        // exit(0);

        uint64_t totalTime = 0, bestTime = ~0ULL;

        // if (0)
        {
            ecs::ComponentsMap map;
            debug_cp();
            // map[ECS_HASH("sample_component")];//ecs::ChildComponent();
            map[ECS_HASH("sample_component")] = SampleComponent();
            map[ECS_HASH("int_variable")] = 13;
            std::string templ2Name;
            ecs::template_t templ2 = create_template(std::move(map), ecs::Template::component_set(), &templ2Name);
            ecs::ComponentsInitializer init;
            init[ECS_HASH("int_variable")] = 27;
            ecs::ComponentsMap map2;
            map2[ECS_HASH("int_variable")] = 17;
            auto eid2 = g_entity_mgr->createEntitySync(templ2, std::move(init), std::move(map2));
            NAU_CORE_DEBUG_LF("getOr {}", g_entity_mgr->getOr(eid2, ECS_HASH("int_variable"), -1));
            while(g_entity_mgr->getNumComponents(eid2) < 0)
            {
                NAU_CORE_DEBUG_LF("wait");
                std::this_thread::sleep_for(2ms);
                // cpujobs::release_done_jobs();
                g_entity_mgr->tick();
            };
            // if (0)
            {
                ecs::ComponentsInitializer init;
                init[ECS_HASH("int_variable")] = 29;
                g_entity_mgr->reCreateEntityFromAsync(eid2, templ2Name.c_str(), ecs::ComponentsInitializer(init));
                // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
                // g_entity_mgr->reCreateEntityFromSync(eid2, templ2, init);
            }
            debug_cp();
            g_entity_mgr->tick();
            debug_cp();

            static constexpr ecs::ComponentDesc comps[] = {
                {ECS_HASH("sample_component"), ecs::ComponentTypeInfo<SampleComponent>()},
                {    ECS_HASH("int_variable"),             ecs::ComponentTypeInfo<int>()}
            };
            ecs::NamedQueryDesc desc{
                "q2",
                std::span(comps),
                nau::ConstSpan<ecs::ComponentDesc>(),
                nau::ConstSpan<ecs::ComponentDesc>(),
                nau::ConstSpan<ecs::ComponentDesc>(),
            };
            auto qid = g_entity_mgr->createQuery(desc);
            ecs::perform_query(g_entity_mgr, qid, [](const ecs::QueryView& components)
                               {
                                   for(auto it = components.begin(), endIt = components.end(); it != endIt; ++it)
                                   {
                                       ECS_LOG("a = {}\n", components.getComponentRW<SampleComponent>(ECS_QUERY_COMP_RW_INDEX(comps, "sample_component"), it).a);
                                       ECS_LOG("int var = {}\n", components.getComponentRW<int>(ECS_QUERY_COMP_RW_INDEX(comps, "int_variable"), it));
                                   }
                               });
            g_entity_mgr->destroyQuery(qid);
            ECS_LOG("a = {}\n", g_entity_mgr->get<SampleComponent>(eid2, ECS_HASH("sample_component")).a);

            ECS_LOG("a = {}\n", g_entity_mgr->getEntityComponentRef(eid2, 0).get<SampleComponent>().a);

            // debug_cp();
            // eid2 = g_entity_mgr->reCreateEntityFromSync(eid2, templ);
            // debug_cp();
            g_entity_mgr->destroyEntityAsync(eid2);
            g_entity_mgr->tick();
            // exit(0);
        }

        testSampleComponent();
        testSharedComponent();
        testCreateObjectOfArray();

        for(int i = 0; i < 500; ++i)  // to move to one chunk, simulate relaxation during many frames
            g_entity_mgr->tick();

        /*{
          SimpleString fname("entities.blk");
          ecs::TemplateRefs trefs;
          ecs::load_templates_blk(nau::ConstSpan<SimpleString>(&fname, 1), trefs, &g_entity_mgr->getMutableTemplateDB().info());
          if (!trefs.empty())
            g_entity_mgr->getMutableTemplateDB().addTemplates(trefs);
          auto eid = g_entity_mgr->createEntityAsync("test_blk");
          g_entity_mgr->destroyEntityAsync(eid);
        }*/
        {
            ecs::ComponentsMap map;
            map[ECS_HASH("int_variable")] = 12;
            map[ECS_HASH("pos")] = Point3(0, 0, 0);
            map[ECS_HASH("vel")] = Vector3(0, 0, 0);
            map[ECS_HASH("tag_sample")] = ecs::Tag();
            map[ECS_HASH("transform")] = TMatrix::identity();
            // map[ECS_HASH("int_component2")] = 10;
            templ = create_template(std::move(map));
            ecs::ComponentsInitializer map2;
            map2[ECS_HASH("transform")] = TMatrix::identity();
            // debug_cp();
            g_entity_mgr->createEntitySync(templ, std::move(map2));
        }
        {
            ecs::ComponentsMap map;
            map[ECS_HASH("pos")] = Point3(0, 0, 0);
            map[ECS_HASH("vel")] = Vector3(1, 0, 0);
            // map[ECS_HASH("int_variable")] = 13;
            templ = create_template(ecs::ComponentsMap(map), "pos");
            auto eid2 = g_entity_mgr->createEntitySync(templ);
            g_entity_mgr->set(eid2, ECS_HASH("pos"), Point3(10, 0, 0));
            debug_cp();
            g_entity_mgr->tick();
            debug_cp();
        }

        static constexpr ecs::ComponentDesc comps[] = {
            {         ECS_HASH("pos"),   ecs::ComponentTypeInfo<Point3>()}, // rw
            {         ECS_HASH("vel"),  ecs::ComponentTypeInfo<Vector3>()}, // ro
            {ECS_HASH("int_variable"),      ecs::ComponentTypeInfo<int>()}, // rq
            {       ECS_HASH("data0"),  ecs::ComponentTypeInfo<TMatrix>()}, // rq
            {  ECS_HASH("tag_sample"), ecs::ComponentTypeInfo<ecs::Tag>()}  // no
        };
        ecs::NamedQueryDesc desc{
            "q1",
            std::span(comps + 0, 1),
            std::span(comps + 1, 1),
            std::span(comps + 2, 2),
            std::span(comps + 4, 1),
        };

        float dt = 0.1;
        prune_cache();
        reft = profile_ref_ticks();
        // ecs::perform_query(g_entity_mgr, desc, [](const ecs::QueryView& ){});
        // NAU_CORE_DEBUG_LF("zero query cost {}us, ", profile_time_usec(reft));
        ecs::QueryId persistentQuery = g_entity_mgr->createQuery(desc);
        NAU_CORE_DEBUG_LF("create query cost {}us", profile_time_usec(reft));
        profileQuery(persistentQuery);

        compare_gets();

        //_exit(0);

        prune_cache();
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("tick in {}us ", profile_time_usec(reft));
        prune_cache();
        reft = profile_ref_ticks();
        ecs::perform_query(g_entity_mgr, persistentQuery, [dt](const ecs::QueryView& qv)
                           {
                               for(auto it = qv.begin(), endIt = qv.end(); it != endIt; ++it)
                               {
                                   // if ((grnd()&31) == 0)
                                   qv.getComponentRW<Point3>(ECS_QUERY_COMP_RW_INDEX(comps, "pos"), it) +=
                                       dt * qv.getComponentRO<Vector3>(ECS_QUERY_COMP_RO_INDEX(comps, "vel"), it);
                                   // NAU_CORE_DEBUG_LF("{:p}", it.getComponentRORaw(0, sizeof(int)));
                                   // NAU_CORE_DEBUG_LF("{}", (*(int*)it.getComponentRORaw(0, sizeof(int))));
                                   // break;
                                   // int_component_calc += *it.getComponentRORaw<int>(1);
                               }
                           },
                           nullptr, 4);
        NAU_CORE_DEBUG_LF("persistent query in {}us", profile_time_usec(reft));

        prune_cache();
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("tick in {}us ", profile_time_usec(reft));
        prune_cache();

        reft = profile_ref_ticks();
        g_entity_mgr->destroyQuery(persistentQuery);
        NAU_CORE_DEBUG_LF("destroy query cost {}us", profile_time_usec(reft));
        ecs::QueryId persistentScalarQuery = g_entity_mgr->createQuery(desc);
        prune_cache();
        reft = profile_ref_ticks();
        // code-gen like
        auto queryFun = [&, dt](const ecs::QueryView& components)
        {
            // NAU_ASSERT(qv.isRW<Point3>(0));NAU_ASSERT(qv.isRO<Point3>(0));
            auto* __restrict pos = ECS_QUERY_COMP_RW_PTR(Point3, kinematics_comps, "pos");
            auto* __restrict posE = pos + components.end();
            pos += components.begin();
            auto* __restrict vel = ECS_QUERY_COMP_RO_PTR(Vector3, comps, "vel");
            // const int * int_component = qv.getComponentRORaw<int>(1, cit);
            do
            {
                // if ((grnd()&31) == 0)
                // pos[i] += dt*vel[i];
                ecs::getRef(pos) += dt * ecs::getRef(vel);
                //*pos += dt * *vel;
                // if (int_component)
                //   int_component_calc += *int_component;
                pos++;
                vel++;
                // if (int_component)
                //   int_component++;
            } while(pos < posE);
        };
        totalTime = 0, bestTime = ~uint64_t(0);
        for(int i = 0; i < Q_CACHE_CNT; ++i)
        {
            prune_cache();
            reft = profile_ref_ticks();
            ecs::perform_query(g_entity_mgr, persistentScalarQuery, queryFun);
            const uint64_t ctime = profile_ref_ticks() - reft;
            bestTime = min(ctime, bestTime);
            totalTime += ctime;
        }
        NAU_CORE_DEBUG_LF("(no cached)query in {}us, best ={}us", double(totalTime) /*/ Q_CACHE_CNT*/,
                          double(bestTime));

        totalTime = 0, bestTime = ~0ULL;
        for(int i = 0; i < ECS_RUNS; ++i)
        {
            reft = profile_ref_ticks();
            ecs::perform_query(g_entity_mgr, persistentScalarQuery, queryFun);
            const uint64_t ctime = profile_ref_ticks() - reft;
            bestTime = min(ctime, bestTime);
            totalTime += ctime;
        }
        NAU_CORE_DEBUG_LF("(cached)query in {}us, best ={}us", double(totalTime) / uint32_t(ECS_RUNS),
                          double(bestTime));
        g_entity_mgr->destroyQuery(persistentScalarQuery);

        {
            static constexpr ecs::ComponentDesc comps[] = {
                {ECS_HASH("pos_vec"), ecs::ComponentTypeInfo<vec4f>()},
                {ECS_HASH("vel_vec"), ecs::ComponentTypeInfo<vec4f>()},
                {ECS_HASH("int_variable"), ecs::ComponentTypeInfo<int>(), ecs::CDF_OPTIONAL}
            };
            ecs::NamedQueryDesc desc{
                "vecq",
                std::span(comps, 1),
                std::span(comps + 1, 2),
                nau::ConstSpan<ecs::ComponentDesc>(),
                nau::ConstSpan<ecs::ComponentDesc>(),
            };
            ecs::QueryId persistentVecQuery = g_entity_mgr->createQuery(desc);
            prune_cache();
            reft = profile_ref_ticks();
            // code-gen like
            ecs::perform_query(g_entity_mgr, persistentVecQuery, [dt](const ecs::QueryView& components)
                               {
                                   auto* __restrict pos = ECS_QUERY_COMP_RW_PTR(vec4f, comps, "pos_vec");  // qv.getComponentRaw_rw<vec4f>(0, cit);
                                   auto* __restrict vel = ECS_QUERY_COMP_RO_PTR(vec4f, comps, "vel_vec");  // qv.getComponentRaw_rw<vec4f>(0, cit);
                                   // const auto* __restrict vel = qv.getComponentRaw_ro<vec4f>(0, cit);
                                   // const int * int_component = qv.getComponentRORaw<int>(1, cit);
                                   for(uint32_t i = 0, ei = components.getEntitiesCount(); i < ei; ++i)
                                   {
                                       *pos = (*vel) * dt + (*pos);
                                       // if (int_component)
                                       //   int_component_calc += *int_component;
                                       pos++;
                                       vel++;
                                       // if (int_component)
                                       //   int_component++;
                                   }
                               });
            NAU_CORE_DEBUG_LF("vec query in {}us", profile_time_usec(reft));
            g_entity_mgr->destroyQuery(persistentVecQuery);
        }

        prune_cache();
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("tick in {}us ", profile_time_usec(reft));
        prune_cache();
        {
            NAU_CORE_DEBUG_LF("before ");
            uint64_t best = ~0u, total = 0;
            for(int i = 0; i < 1000; ++i)
            {
                reft = profile_ref_ticks();
                g_entity_mgr->update(ecs::UpdateStageInfoAct(0.1, 0.1));
                const uint64_t ct = profile_ref_ticks() - reft;
                best = min(best, ct);
                total += ct;
            }
            NAU_CORE_DEBUG_LF("best update in {}us ", double(best));
        }
        prune_cache();
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("tick in {}us ", profile_time_usec(reft));
        prune_cache();
        reft = profile_ref_ticks();
        int ret = 0;
        // for (int j = 0; j < 100000; ++j)
        for(int j = 0; j < 100; ++j)
            for(int i = 0; i < TESTS; ++i)
                ret += g_entity_mgr->get<int>(eid.data()[i], ECS_HASH("int_variable"));
        NAU_CORE_DEBUG_LF("int_component = {} in {} us", ret, profile_time_usec(reft) / 100.);

        prune_cache();
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("tick in {}us ", profile_time_usec(reft));

        prune_cache();
        reft = profile_ref_ticks();
        // for (int i = TESTS-1; i >= 0; --i)
        for(int i = 0; i < TESTS; ++i)
            g_entity_mgr->destroyEntityAsync(eid.data()[i]);
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("destroy = {} us", profile_time_usec(reft));

        prune_cache();
        reft = profile_ref_ticks();
        g_entity_mgr->tick();
        NAU_CORE_DEBUG_LF("tick in {}us ", profile_time_usec(reft));

        std::vector<Point3> posOld, pos8;
        std::vector<Vector3> vel8;
        posOld.resize(TESTS, Point3(0, 0, 0));
        pos8.resize(TESTS, Point3(1, 0, 0));
        vel8.resize(TESTS, Vector3(0, 1, 0));
        {
            prune_cache();
            auto testQ = [&pos8, &vel8, dt]()
            {
                Point3* /*__restrict*/ pos = (Point3*)(pos8.data());                // with __restrict it is theoretical limit. But noone really writes
                                                                                    // __restrict, so we omit it intentionally
                const Vector3* /*__restrict*/ vel = (const Vector3*)(vel8.data());  // with __restrict it is theoretical limit. But noone really
                                                                                    // writes __restrict, so we omit it intentionally
                for(int i = 0; i < TESTS; ++i)
                    pos[i] += dt * vel[i];
            };
            totalTime = 0, bestTime = ~uint64_t(0);
            for(int i = 0; i < Q_CACHE_CNT; ++i)
            {
                prune_cache();
                reft = profile_ref_ticks();
                testQ();
                const uint64_t ctime = profile_ref_ticks() - reft;
                bestTime = min(ctime, bestTime);
                totalTime += ctime;
            }
            NAU_CORE_DEBUG_LF("(no cache) speed limit avg {}us, best ={}us", double(totalTime) /*/ Q_CACHE_CNT*/,
                              double(bestTime));

            totalTime = 0, bestTime = ~0ULL;
            for(int i = 0; i < Q_CNT; ++i)
            {
                reft = profile_ref_ticks();
                testQ();
                const uint64_t ctime = profile_ref_ticks() - reft;
                bestTime = min(ctime, bestTime);
                totalTime += ctime;
            }
            NAU_CORE_DEBUG_LF("(cached) speed limit avg {}us, best ={}us", double(totalTime) / uint32_t(Q_CNT),
                              double(bestTime));
        }
        {
            std::vector<TestEntity> tests(TESTS);
            prune_cache();
            auto testQ = [&tests, dt]()
            {
                for(auto& t : tests)
                    t.p += dt * t.v;
            };
            reft = profile_ref_ticks();
            uint64_t result_time = 0;
            for(int i = 0; i < Q_CACHE_CNT; ++i)
            {
                prune_cache();
                reft = profile_ref_ticks();
                testQ();
                result_time += profile_ref_ticks() - reft;
            }
            NAU_CORE_DEBUG_LF("(no cache)speed limit with entity {}us", double(result_time) /*/ Q_CACHE_CNT*/);

            reft = profile_ref_ticks();
            for(int i = 0; i < Q_CNT; ++i)
                testQ();
            NAU_CORE_DEBUG_LF("(cached) speed limit with entity avg {}us", double(profile_ref_ticks() - reft) / uint32_t(Q_CNT));
        }
        {
            std::vector<std::unique_ptr<TestEntity>> tests;
            reft = profile_ref_ticks();
            tests.resize(TESTS);
            for(auto& t : tests)
                t.reset(new TestEntity(&t - tests.begin()));
            NAU_CORE_DEBUG_LF("best possible (single alloc + ptrs) create time = {} us", profile_time_usec(reft));
            tests.clear();
            tests.shrink_to_fit();
            reft = profile_ref_ticks();
            for(int i = 0; i < TESTS; ++i)
                tests.emplace_back(new TestEntity(i));
            ::nau::diag_detail::InplaceLogData{
                ::nau::diag::LogLevel::Info, ::nau::diag::SourceInfo{::std::string_view{__FUNCTION__}, ::std::string_view{"D:\\NauPrototype\\engine\\core\\modules\\dagorECS\\tests\\test_ecs\\test_base.cpp"}, static_cast<unsigned>(1356)}
            }({"ecs"}, "best possible (grow ptrs) create time = {} us", profile_time_usec(reft));
            // we shouldn't assume all of them were allocated simultaneously, so allocator returns ordered. that's optimistic scenario
            // to simulate real environment, shuffle data
            struct Rand
            {
                uint32_t operator()(uint32_t n)
                {
                    return (uint32_t)(rand() % n);
                }
            };
            Rand r;
            for (size_t i = 1; i < tests.size(); ++i)
                std::swap(tests[i], tests[r(i + 1)]);
            prune_cache();
            auto testQ = [&tests, dt]()
            {
                for(auto& t : tests)
                    t->p += dt * t->v;
            };
            auto testVQ = [&tests, dt]()
            {
                for(auto& t : tests)
                    t->update(dt);
            };
            reft = profile_ref_ticks();
            int64_t result_time = 0;
            for(int i = 0; i < Q_CACHE_CNT; ++i)
            {
                prune_cache();
                reft = profile_ref_ticks();
                testQ();
                result_time += profile_ref_ticks() - reft;
            }
            NAU_CORE_DEBUG_LF("(no cache)speed limit with ptr entity {}us", double(result_time) /*/ Q_CACHE_CNT*/);

            reft = profile_ref_ticks();
            for(int i = 0; i < Q_CNT; ++i)
                testQ();
            NAU_CORE_DEBUG_LF("(cached) speed limit with ptr entity avg {}us", double(profile_ref_ticks() - reft) / uint32_t(Q_CNT));

            reft = profile_ref_ticks();
            result_time = 0;
            for(int i = 0; i < Q_CACHE_CNT; ++i)
            {
                prune_cache();
                reft = profile_ref_ticks();
                testVQ();
                result_time += profile_ref_ticks() - reft;
            }
            NAU_CORE_DEBUG_LF("(no cache)speed limit with virtual call ptr entity {}us", double(result_time) /*/ Q_CACHE_CNT*/);

            reft = profile_ref_ticks();
            for(int i = 0; i < Q_CNT; ++i)
                testVQ();
            NAU_CORE_DEBUG_LF("(cached) speed limit with virtual call ptr entity avg {}us",
                              double(profile_ref_ticks() - reft) / uint32_t(Q_CNT));
            {
                reft = profile_ref_ticks();
                tests.clear();
                tests.shrink_to_fit();
                NAU_CORE_DEBUG_LF("single destroy ptr entity avg {}us", double(profile_ref_ticks() - reft));
            }
        }
        /*{
          prune_cache();
          reft = profile_ref_ticks();
          Point3 * __restrict pos = (Point3 *__restrict )(pos8.data());
          Point3 * __restrict vel = (Point3 *__restrict )(vel8.data());
          int changed = 0;
          for (int i = 0; i < TESTS; ++i)
          {
            auto opos = pos[i];
            pos[i] += dt*vel[i];
            if (pos[i] != opos)
              changed++;
          }
          NAU_CORE_DEBUG_LF("track change inplace {}us, ret= {}", profile_time_usec(reft), changed);
        }

        {
          prune_cache();
          reft = profile_ref_ticks();
          Point3 * __restrict pos = (Point3 *__restrict )(pos8.data());
          Point3 * __restrict opos = (Point3 *__restrict )(posOld.data());
          int changed = 0;
          for (int i = 0; i < TESTS; ++i)
            if (pos[i] != opos[i])
              changed++;
          NAU_CORE_DEBUG_LF("track change stream {}us, ret= {}", profile_time_usec(reft), changed);
        }*/
        // NAU_CORE_DEBUG_LF("float_component = {}", g_entity_mgr->get<float>(eid, ECS_HASH("float_component")));
        TIME_PROFILER_SHUTDOWN();

        g_entity_mgr.demandDestroy();
        ecs::setECSResourceManager(nullptr);
        getApplication().stop();
        while(app->step())
        {
            std::this_thread::sleep_for(50ms);
        }
    }

}  // namespace nau::test
