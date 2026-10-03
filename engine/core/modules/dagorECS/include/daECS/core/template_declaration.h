// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <utility>

#include "daECS/core/componentTypes.h"
#include "daECS/core/entityManager.h"
#include "nau/utils/for_each.h"

namespace ecs
{

    template_t inline CreateTemplate(const char* name, ecs::ComponentsMap&& map, nau::ConstSpan<ecs::template_t> parents)
    {
        auto res = g_entity_mgr->addTemplate(
            ecs::Template(name,
                          std::move(map),
                          {},
                          ecs::Template::component_set(),
                          ecs::Template::component_set(),
                          false),
            parents);
        NAU_ASSERT(res == ecs::TemplateDB::AR_OK);
        return g_entity_mgr->instantiateTemplate(g_entity_mgr->buildTemplateIdByName(name));
    }
}  // namespace ecs

#define ECS_TEMPLATE_NAME(TemplateName) TemplateName
#define ECS_TEMPLATE_FIRST(TemplateName, ...) TemplateName
#define ECS_TEMPLATE_REST(TemplateName, ...) __VA_ARGS__
#define ECS_TEMPLATE_NAME_LIST(TemplateName) , ECS_TEMPLATE_NAME(TemplateName)
#define ECS_TEMPLATE_ID(TemplateName) ECS_TEMPLATE_NAME(TemplateName)::getTemplateId()
#define ECS_TEMPLATE_ID_LIST(TemplateName) ECS_TEMPLATE_ID(TemplateName),

#define BEGIN_ECS_TEMPLATE(TemplateName, ...)                                                                         \
    struct ECS_TEMPLATE_NAME(TemplateName) :                                                                          \
        public ::ecs::EntityId                                                                                        \
    {                                                                                                                 \
        ECS_TEMPLATE_NAME(TemplateName)                                                                               \
        ()                                                                                                            \
        {                                                                                                             \
            NAU_ASSERT(g_entity_mgr);                                                                                 \
            static_cast<::ecs::EntityId&>(*this) = g_entity_mgr->createEntitySync<ECS_TEMPLATE_NAME(TemplateName)>(); \
        }                                                                                                             \
        ~ECS_TEMPLATE_NAME(TemplateName)()                                                                            \
        {                                                                                                             \
            NAU_ASSERT(g_entity_mgr);                                                                                 \
            NAU_ASSERT(g_entity_mgr->doesEntityExist(*this));                                                         \
            NAU_ASSERT(g_entity_mgr->destroyEntityAsync(*this));                                                      \
        }                                                                                                             \
        static inline ::ecs::template_t getTemplateId()                                           \
        {                                                                                                             \
            static ::ecs::template_t g_tid = ::ecs::INVALID_TEMPLATE_INDEX;                                           \
            if(g_tid == ::ecs::INVALID_TEMPLATE_INDEX)                                                                \
            {                                                                                                         \
                const char* templateName = #TemplateName;                                                             \
                static const ::ecs::Template::ParentsList parents = {                                                 \
                    __VA_OPT__(NAU_FOR_EACH(ECS_TEMPLATE_ID_LIST, __VA_ARGS__))};                                     \
                ::ecs::ComponentsMap map;

#define ADD_ECS_COMPONENT(name, type, ...) map[ECS_HASH(#name)] = type(__VA_ARGS__);

#define END_ECS_TEMPLATE(TemplateName)                                    \
    g_tid = ::ecs::CreateTemplate(templateName, std::move(map), parents); \
    }                                                                     \
    return g_tid;                                                         \
    }                                                                     \
    }                                                                     \
    ;

BEGIN_ECS_TEMPLATE(TemplateExample)
ADD_ECS_COMPONENT(templateExample, ecs::Tag)
END_ECS_TEMPLATE(ExampleTemplate)

BEGIN_ECS_TEMPLATE(DerivedTemplateExample, TemplateExample)
ADD_ECS_COMPONENT(derivedTemplateExample, ecs::Tag)
END_ECS_TEMPLATE(ExampleTemplate)
