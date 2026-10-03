// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "render/daBfg/registry.h"

#include <utility>

#include "dabfg/frontend/internalRegistry.h"
#include "nau/diag/logging.h"

namespace dabfg
{

Registry::Registry(NodeNameId node, InternalRegistry *reg) : NameSpaceRequest(reg->knownNames.getParent(node), node, reg) {}

Registry Registry::orderMeBefore(const char *name)
{
  const auto beforeId = registry->knownNames.addNameId<NodeNameId>(nameSpaceId, name);

  if (DAGOR_LIKELY(beforeId != NodeNameId::Invalid))
    registry->nodes[nodeId].followingNodeIds.insert(beforeId);
  else
    NAU_LOG_ERROR("FG: node {} tries to order it before a non-existent node {}, skipping this ordering.",
      registry->knownNames.getName(nodeId), name);

  return *this;
}

Registry Registry::orderMeBefore(std::initializer_list<const char *> names)
{
  for (auto name : names)
    orderMeBefore(name);
  return *this;
}

Registry Registry::orderMeAfter(const char *name)
{
  const auto afterId = registry->knownNames.addNameId<NodeNameId>(nameSpaceId, name);

  if (DAGOR_LIKELY(afterId != NodeNameId::Invalid))
    registry->nodes[nodeId].precedingNodeIds.insert(afterId);
  else
    NAU_LOG_ERROR("FG: node {} tries to order it after a non-existent node {}, skipping this ordering.", registry->knownNames.getName(nodeId),
      name);

  return *this;
}

Registry Registry::orderMeAfter(std::initializer_list<const char *> names)
{
  for (auto name : names)
    orderMeAfter(name);
  return *this;
}

Registry Registry::setPriority(priority_t prio)
{
  registry->nodes[nodeId].priority = prio;
  return *this;
}

Registry Registry::multiplex(multiplexing::Mode mode)
{
  registry->nodes[nodeId].multiplexingMode = mode;
  return *this;
}

Registry Registry::executionHas(SideEffects sideEffect)
{
  registry->nodes[nodeId].sideEffect = sideEffect;
  return *this;
}

StateRequest Registry::requestState() { return {registry, nodeId}; }

VirtualPassRequest Registry::requestRenderPass() { return {nodeId, registry}; }

NameSpaceRequest Registry::root() { return {registry->knownNames.root(), nodeId, registry}; }

VirtualResourceCreationSemiRequest Registry::create(const char *name, History history)
{
  auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);

  auto &res = registry->resources.get(resId);
  res.history = history;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});

  return {{resId, false}, nodeId, registry};
}

detail::ResUid Registry::registerTexture2dImpl(const char *name, dabfg::ExternalResourceProvider &&p)
{
  const auto resId = registry->knownNames.addNameId<ResNameId>(nameSpaceId, name);
  auto &res = registry->resources.get(resId);

  res.creationInfo = std::move(p);
  res.type = ResourceType::Texture;

  registry->nodes[nodeId].createdResources.insert(resId);
  registry->nodes[nodeId].resourceRequests.emplace(resId, ResourceRequest{ResourceUsage{Access::READ_WRITE}});

  return {resId, false};
}

} // namespace dabfg
