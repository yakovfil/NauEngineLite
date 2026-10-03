// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <daECS/core/entitySystem.h>

#include <algorithm>

namespace ecs
{

inline bool allCompsAreOptional(const EntitySystemDesc *es)
{
  auto isOptPred = [](const ecs::ComponentDesc &cd) { return (cd.flags & CDF_OPTIONAL) != 0 || cd.name == ECS_HASH("eid").hash; };
  return es->componentsRQ.empty() && std::all_of(es->componentsRW.begin(), es->componentsRW.end(), isOptPred) &&
         std::all_of(es->componentsRO.begin(), es->componentsRO.end(), isOptPred);
}

} // namespace ecs