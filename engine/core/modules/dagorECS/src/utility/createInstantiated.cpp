// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/entityId.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/internal/typesAndLimits.h>
#include <daECS/core/template.h>
#include <daECS/utility/createInstantiated.h>

#include <utility>

namespace ecs
{

EntityId createInstantiatedEntitySync(EntityManager &mgr, const char *name, ComponentsInitializer &&initializer)
{
  const template_t t = mgr.getTemplateDB().getInstantiatedTemplateByName(name);
  if (NAU_UNLIKELY(t == ecs::INVALID_TEMPLATE_INDEX))
  {
    logerr("Template '{}' hasn't been instantiated", name);
    return INVALID_ENTITY_ID;
  }
  return mgr.createEntitySync(name, std::move(initializer));
}

} // namespace ecs