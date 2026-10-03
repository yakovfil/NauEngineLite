// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <span>

#include "dabfg/frontend/internalRegistry.h"
#include "dabfg/id/idNameResolver.h"
#include "nau/utils/fixed_map.h"

namespace dabfg
{

// Encapsulates our now non-trivial name resolution logic, both
// lookup in parent namespaces and slot resolution.
class NameResolver
{
public:
  NameResolver(const InternalRegistry &reg) : registry{reg} {}

  void update();

  template <class T>
  T resolve(T name_id) const;

  // Returns the list of unresolved resource ids that were requested by node node_id
  // but resulted in a single resolved res_id request. Note that this does not
  // account for history requests.
  std::span<ResNameId const> unresolve(NodeNameId node_id, ResNameId res_id) const;

  // Iterate over all resolved ids and unresolved res id lists corresponding to
  // them that were requested by the node node_id. This iterates both history
  // and regular requests.
  template <class F>
  void iterateInverseMapping(NodeNameId node_id, F &&callback) const
  {
    for (const auto &[resolvedResId, unresolvedResIds] : inverseMapping[node_id])
      callback(false, resolvedResId, unresolvedResIds);
    for (const auto &[resolvedResId, unresolvedResIds] : inverseHistoryMapping[node_id])
      callback(true, resolvedResId, unresolvedResIds);
  }

private:
  void updateMapping();
  void updateInverseMapping();

  const InternalRegistry &registry;
  IdNameResolver<NameSpaceNameId, ResNameId, NodeNameId, AutoResTypeNameId> resolver;

  using ResolvedToUnresolved = nau::FixedMap<ResNameId, dag::RelocatableFixedVector<ResNameId, 4>, 16>;
  // Mapping (node, resolved res id) -> unresolved res Ids
  IdIndexedMapping<NodeNameId, ResolvedToUnresolved> inverseMapping;
  IdIndexedMapping<NodeNameId, ResolvedToUnresolved> inverseHistoryMapping;
};

} // namespace dabfg