// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#include "nodeStateDeltas.h"

#include <algorithm>
#include <functional>
#include <iterator>
#include <optional>
#include <utility>

#include "nau/memory/scoped_standard_allocator.h"
#include "nau/utils/sorted_vector.h"

namespace dag
{

// TODO: move to RelocatableFixedVector
template <class T, size_t first_size, size_t second_size>
static bool operator==(const RelocatableFixedVector<T, first_size> &first, const RelocatableFixedVector<T, second_size> &second)
{
  if (first.size() != second.size())
    return false;
  return std::equal(first.begin(), first.end(), second.begin());
}

} // namespace dag

namespace dabfg
{

// TODO: when C++20 comes, purge all of this and replace with default operator==
// NOTE: ADL does not find these operators unless they are in the same
// namespace as the types they operate on.
namespace intermediate
{

static bool operator==(const VrsState &first, const VrsState &second)
{
  return first.rateX == second.rateX && first.rateY == second.rateY && first.rateTexture == second.rateTexture &&
         first.vertexCombiner == second.vertexCombiner && first.pixelCombiner == second.pixelCombiner;
}

static bool operator==(const SubresourceRef &first, const SubresourceRef &second)
{
  return first.resource == second.resource && first.mipLevel == second.mipLevel && first.layer == second.layer;
}

static bool operator==(const RenderPass &first, const RenderPass &second)
{
  return first.colorAttachments == second.colorAttachments && first.depthAttachment == second.depthAttachment &&
         first.depthReadOnly == second.depthReadOnly;
}

static bool operator<(const Binding &first, const Binding &second)
{
  if (first.type != second.type)
    return first.type < second.type;
  if (first.resource != second.resource)
    return first.resource < second.resource;
  if (first.history != second.history)
    return first.history < second.history;
  if (first.projectedTag != second.projectedTag)
    return first.projectedTag < second.projectedTag;
  const auto projector1 = reinterpret_cast<void *>(first.projector);
  const auto projector2 = reinterpret_cast<void *>(second.projector);
  if (projector1 != projector2)
    return projector1 < projector2;
  return false;
}

} // namespace intermediate

namespace sd
{

struct DeltaCalculator
{
  // Use this overload set to add special processing for fields
  // that change type.

  template <class U>
  U delta(const U &current, const U &previous)
  {
    G_UNUSED(previous);
    return current;
  }

  shaders::UniqueOverrideStateId delta(const std::optional<shaders::OverrideState>& current,
                                       const std::optional<shaders::OverrideState>&)
  {
    return current.has_value() ? shaders::overrides::create(*current) : shaders::OverrideStateId{};
  }

  VrsStateDelta delta(const std::optional<intermediate::VrsState>& current, const std::optional<intermediate::VrsState>&)
  {
    return current.has_value() ? VrsStateDelta{*current} : StopVrs{};
  }

  RenderPassDelta delta(const std::optional<intermediate::RenderPass>& current, const std::optional<intermediate::RenderPass>&)
  {
    return current.has_value() ? RenderPassDelta{*current} : FinishRenderPass{};
  }

  intermediate::BindingsMap delta(const intermediate::BindingsMap &old_binding, const intermediate::BindingsMap &new_binding)
  {
    //FRAMEMEM_VALIDATE;

    // Pre-allocate all framemem data for graceful cleanup
    using BindingToSv = std::pair<int, intermediate::Binding>;
    using TmpBindingsSet = nau::SortedVectorSet<BindingToSv, std::less<BindingToSv>, nau::FrameStdAllocator<BindingToSv>>;
    TmpBindingsSet newSet, oldSet;
    for (const auto& [key, value] : new_binding)
        newSet.emplace(key, value);
    for (const auto& [key, value] : old_binding)
        oldSet.emplace(key, value);

    using TmpShaderVarBindingsMap = nau::SortedVectorMap<int, intermediate::Binding, std::less<int>, nau::FrameStdAllocator<std::pair<int, intermediate::Binding>>>;
    TmpShaderVarBindingsMap oldWithoutNew;
    oldWithoutNew.reserve(oldSet.size());
    TmpShaderVarBindingsMap result;
    result.reserve(newSet.size() + oldSet.size());

    std::set_difference(newSet.begin(), newSet.end(), oldSet.begin(), oldSet.end(), std::inserter(result, result.begin()));
    std::set_difference(oldSet.begin(), oldSet.end(), newSet.begin(), newSet.end(), std::inserter(oldWithoutNew, oldWithoutNew.begin()));

    // Unbind stuff
    for (auto &[k, v] : oldWithoutNew)
      if (result.find(k) == result.end())
          result[k] = {v.type, std::nullopt, false, v.projectedTag};

    // Transfer to regular heap memory
    return {result.begin(), result.end()};
  }

  ShaderBlockLayersInfoDelta delta(const intermediate::ShaderBlockBindings &old_info,
    const intermediate::ShaderBlockBindings &new_info)
  {
    ShaderBlockLayersInfoDelta result;
    result.objectLayer = new_info.objectLayer != old_info.objectLayer ? std::optional<int>(new_info.objectLayer) : std::nullopt;
    result.sceneLayer = new_info.sceneLayer != old_info.sceneLayer ? std::optional<int>(new_info.sceneLayer) : std::nullopt;
    result.frameLayer = new_info.frameLayer != old_info.frameLayer ? std::optional<int>(new_info.frameLayer) : std::nullopt;
    return result;
  }
};

NodeStateDelta getStateDelta(const intermediate::RequiredNodeState &first, const intermediate::RequiredNodeState &second)
{
  NodeStateDelta result;
  DeltaCalculator calculator;

#define DELTA(field)                                                     \
  do                                                                     \
    if (!(first.field == second.field))                                  \
      result.field.emplace(calculator.delta(second.field, first.field)); \
  while (false)

  DELTA(wire);
  DELTA(vrs);
  DELTA(pass);
  DELTA(shaderOverrides);

#undef DELTA

  result.bindings = calculator.delta(first.bindings, second.bindings);
  result.shaderBlockLayers = calculator.delta(first.shaderBlockLayers, second.shaderBlockLayers);

  return result;
}

NodeStateDeltas calculate_per_node_state_deltas(const intermediate::Graph &graph)
{
  NodeStateDeltas result;
  result.reserve(graph.nodeStates.size() + 1);

  if (graph.nodeStates.empty())
  {
    result.appendNew();
    return result;
  }

  result.appendNew(getStateDelta({}, graph.nodeStates.front()));
  for (uint32_t i = 1; i < graph.nodeStates.size(); ++i)
  {
    const auto &prevState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i - 1)];
    const auto &currState = graph.nodeStates[static_cast<intermediate::NodeIndex>(i)];
    result.appendNew(getStateDelta(prevState, currState));
  }
  result.appendNew(getStateDelta(graph.nodeStates.back(), {}));

  return result;
}

} // namespace sd

} // namespace dabfg
