// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include <daECS/core/entityManager.h>

#include <limits>
#include <utility>
#include <vector>

#include "nau/utils/dynamic_bit_vector.h"

namespace ecs
{

uint32_t EntityManager::defragTemplates()
{
  NAU_ASSERT_RETURN(!isConstrainedMTMode(), 0);
  NAU_ASSERT_RETURN(nestedQuery == 0, 0);
  nau::DynamicBitVector<> templUsed(templates.size(), false);
  for (const auto &ei : entDescs.entDescs)
  {
    if (ei.archetype == INVALID_ARCHETYPE)
      continue;
    templUsed.set(ei.template_id, true);
  }

  for (auto &ci : delayedCreationQueue)
    for (auto &cr : ci)
      if (!cr.isToDestroy())
        templUsed.set(cr.templ, true);
  NAU_ASSERT(templUsed.size() == templates.size());
  std::vector<template_t, std::allocator<template_t>> remapTemplates(templates.size());
  uint32_t used = 0;
  for (uint32_t i = 0, e = templates.size(); i != e; ++i)
  {
    if (templUsed.test(i, false))
      remapTemplates[i] = used++;
    else
      remapTemplates[i] = INVALID_TEMPLATE_INDEX;
  }
  const uint32_t unused = templates.size() - used;
  if (!unused)
    return 0;
  // defrag templates
  // remove unused templates
  templates.remap(remapTemplates.data(), used, true, archetypes, dataComponents, componentTypes);

  // remap all old templates
  for (auto &t : templateDB.instantiatedTemplates)
  {
    if (t.t == INVALID_TEMPLATE_INDEX)
      continue;
    NAU_ASSERT(t.t < remapTemplates.size());
    t.t = remapTemplates[t.t];
  }
  for (auto &ci : delayedCreationQueue)
    for (auto &cr : ci)
      if (!cr.isToDestroy())
      {
        NAU_ASSERT(cr.templ < remapTemplates.size());
        cr.templ = remapTemplates[cr.templ];
        NAU_ASSERT(cr.templ != INVALID_TEMPLATE_INDEX && cr.templ < templates.size());
      }
  for (auto &ei : entDescs.entDescs)
  {
    if (ei.archetype == INVALID_ARCHETYPE)
      continue;
    NAU_ASSERT(ei.template_id < remapTemplates.size() && remapTemplates[ei.template_id] != INVALID_TEMPLATE_INDEX &&
                   remapTemplates[ei.template_id] < templates.size(),
               "eid={} template was {} -> {}, total new {}", make_eid(&ei - entDescs.entDescs.data(), ei.generation), ei.template_id,
               remapTemplates[ei.template_id], templates.size());
    ei.template_id = remapTemplates[ei.template_id];
  }
  // todo: send to net broadcast event with changes
  return unused;
}

uint32_t EntityManager::defragArchetypes()
{
  if (!archetypes.size())
    return 0;
  if (!defragTemplates())
    return 0;
  performTrackChanges(true); // try to track changes first. only needed for scheduled archetype changes. We remap scheduled anyway, but
                             // this could create more archetypes, so just do it
  nau::DynamicBitVector<> archUsed(archetypes.size(), false);

  for (uint32_t i = 0, e = templates.size(); i != e; ++i)
    archUsed.set(templates.getTemplate(i).archetype, true);

  uint32_t used = 0;
  std::vector<archetype_t, std::allocator<archetype_t>> remapArchetypes(archetypes.size());
  for (uint32_t i = 0, e = remapArchetypes.size(); i != e; ++i)
  {
    if (archUsed.test(i, false))
    {
      remapArchetypes[i] = used++;
    }
    else
      remapArchetypes[i] = INVALID_ARCHETYPE;
  }

  const uint32_t unused = archetypes.size() - used;
  if (!unused)
    return 0;
  // remove unused archetypes
  archetypes.remap(remapArchetypes.data(), used);
  convertArchetypeScheduledChanges();
  if (!archetypeTrackingQueue.empty())
  {
    TrackedChangeArchetype archetypeTrackingQueue2;
    for (auto scheduled : archetypeTrackingQueue)
    {
      static_assert(sizeof(archetype_t) + sizeof(component_index_t) <= sizeof(scheduled));
      const archetype_t archetype = (scheduled & std::numeric_limits<archetype_t>::max());
      const component_index_t cidx = (scheduled >> (8 * sizeof(archetype_t)));
      if (remapArchetypes[archetype] != INVALID_ARCHETYPE)
        archetypeTrackingQueue2.insert(remapArchetypes[archetype] | (cidx << (8 * sizeof(archetype_t))));
    }
    std::swap(archetypeTrackingQueue, archetypeTrackingQueue2);
  }

  // remap archetypes
  for (uint32_t i = 0, e = templates.size(); i != e; ++i)
  {
    const archetype_t arch = remapArchetypes[templates.getTemplate(i).archetype];
    templates.getTemplate(i).archetype = arch;
    NAU_ASSERT(arch != INVALID_ARCHETYPE);
  }
  for (auto &ei : entDescs.entDescs)
  {
    if (ei.archetype == INVALID_ARCHETYPE)
      continue;
    const archetype_t arch = remapArchetypes[ei.archetype];
    ei.archetype = arch;
    NAU_ASSERT(arch != INVALID_ARCHETYPE);
  }
  invalidatePersistentQueries();
  updateAllQueriesInternal();
  return unused;
}

} // namespace ecs