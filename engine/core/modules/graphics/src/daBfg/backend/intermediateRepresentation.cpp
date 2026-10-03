// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "intermediateRepresentation.h"

#include <span>
#include <utility>
#include <vector>

#include "dabfg/id/idExtentsFinder.h"
#include "dabfg/id/idRange.h"
#include "nau/math/math.h"
#include "nau/memory/scoped_standard_allocator.h"
#include "nau/utils/dynamic_bit_vector.h"
#include "nau/utils/enum_utils.h"

namespace dabfg
{

namespace intermediate
{

    void Graph::choseSubgraph(std::span<const NodeIndex> old_to_new_index_mapping)
    {
        NAU_ASSERT(old_to_new_index_mapping.size() == nodes.size());

        // Jump through hoops in order to do this without any persistent
        // allocations

        // FRAMEMEM_VALIDATE;

        size_t newNodeCount = 0;
        for (auto [i, _] : nodes.enumerate())
            if (old_to_new_index_mapping[i] != NODE_NOT_MAPPED)
                ++newNodeCount;

        // Sanity check: new indexing should not have gaps
        for (auto [i, _] : nodes.enumerate())
            if (old_to_new_index_mapping[i] != NODE_NOT_MAPPED)
                NAU_ASSERT(old_to_new_index_mapping[i] < newNodeCount);

        const auto choseSoa = [](auto& old_data, size_t new_count, const auto& mapping, const auto unmappedSentinel)
        {
            using Mapping = std::decay_t<decltype(old_data)>;
            IdIndexedMapping<typename Mapping::index_type, typename Mapping::value_type, nau::FrameStdAllocator<typename Mapping::value_type>> newData;
            newData.resize(new_count);
            for (auto [i, data] : old_data.enumerate())
                if (mapping[i] != unmappedSentinel)
                    newData[mapping[i]] = std::move(data);

            old_data.resize(new_count);
            std::move(newData.begin(), newData.end(), old_data.begin());
        };

        choseSoa(nodes, newNodeCount, old_to_new_index_mapping, NODE_NOT_MAPPED);
        choseSoa(nodeStates, newNodeCount, old_to_new_index_mapping, NODE_NOT_MAPPED);
        choseSoa(nodeNames, newNodeCount, old_to_new_index_mapping, NODE_NOT_MAPPED);

        // We need to fixup all node -> node references
        for (auto [i, node] : nodes.enumerate())
        {
            auto oldPreds = std::move(node.predecessors);
            node.predecessors.clear();
            for (const auto pred : oldPreds)
                if (old_to_new_index_mapping[pred] != NODE_NOT_MAPPED)
                    node.predecessors.insert(old_to_new_index_mapping[pred]);
        }

        std::vector<ResourceIndex, nau::FrameStdAllocator<ResourceIndex>> resourceMapping(resources.size(), RESOURCE_NOT_MAPPED);

        {
            std::vector<Resource, nau::FrameStdAllocator<Resource>> newResources;
            newResources.reserve(resources.size());
            for (auto& node : nodes)
                for (auto& req : node.resourceRequests)
                {
                    if (resourceMapping[req.resource] == RESOURCE_NOT_MAPPED)
                    {
                        resourceMapping[req.resource] = static_cast<ResourceIndex>(newResources.size());
                        newResources.emplace_back(std::move(resources[req.resource]));
                    }
                    req.resource = resourceMapping[req.resource];
                }
            resources.resize(newResources.size());
            std::move(newResources.begin(), newResources.end(), resources.begin());
        }

        choseSoa(resourceNames, resources.size(), resourceMapping, RESOURCE_NOT_MAPPED);

        const auto updateResourceIndex = [&resourceMapping](ResourceIndex& index)
        {
            NAU_ASSERT(resourceMapping[index] != RESOURCE_NOT_MAPPED);
            index = resourceMapping[index];
        };

        // We also need to fix up node -> resource references
        for (auto& state : nodeStates)
        {
            if (state.vrs)
                updateResourceIndex(state.vrs->rateTexture);

            for (const auto& [id, binding] : state.bindings)
                if (binding.resource.has_value())
                {
                    // FIXME: this is a dumb hack due to iteration being const only
                    // for dag::FixedVectorMap, should probably fix it at some point...
                    updateResourceIndex(*state.bindings[id].resource);
                }

            if (state.pass)
            {
                if (state.pass->depthAttachment.has_value())
                    updateResourceIndex(state.pass->depthAttachment->resource);
                for (auto& color : state.pass->colorAttachments)
                    if (color.has_value())
                        updateResourceIndex(color->resource);
            }
        }
}

Mapping Graph::calculateMapping()
{
  // Jump through hoops to ensure a single allocation

  IdExtentsFinder<MultiplexingIndex> multiIdxExtents;

  IdExtentsFinder<NodeNameId> nodeNameIdExtents;
  for (const auto &node : nodes)
  {
    multiIdxExtents.update(node.multiplexingIndex);
    nodeNameIdExtents.update(node.frontendNode);
  }

  IdExtentsFinder<ResNameId> resNameIdExtents;
  for (const auto &res : resources)
  {
    multiIdxExtents.update(res.multiplexingIndex);
    for (auto resNameId : res.frontendResources)
      resNameIdExtents.update(resNameId);
  }

  Mapping result{nodeNameIdExtents.get(), resNameIdExtents.get(), multiIdxExtents.get()};

  for (auto [i, res] : resources.enumerate())
    for (auto resNameId : res.frontendResources)
      result.mapRes(resNameId, res.multiplexingIndex) = i;

  for (auto [i, node] : nodes.enumerate())
    result.mapNode(node.frontendNode, node.multiplexingIndex) = i;

  // NOTE: This is a bit fragile. The frontend multiplexing code has to
  // guarantee that in case of undermultiplexed nodes/resources each
  // "empty" mapping slot should be mapped to the nearest previous mapped
  // slot.

  for (uint32_t i = 1; i < result.multiplexingExtent(); ++i)
  {
    const auto currMultiIdx = static_cast<MultiplexingIndex>(i);
    const auto prevMultiIdx = static_cast<MultiplexingIndex>(i - 1);

    for (auto resNameId : IdRange<ResNameId>(result.mappedResNameIdCount()))
      if (!result.wasResMapped(resNameId, currMultiIdx))
        result.mapRes(resNameId, currMultiIdx) = result.mapRes(resNameId, prevMultiIdx);

    for (auto nodeNameId : IdRange<NodeNameId>(result.mappedNodeNameIdCount()))
      if (!result.wasNodeMapped(nodeNameId, currMultiIdx))
        result.mapNode(nodeNameId, currMultiIdx) = result.mapNode(nodeNameId, prevMultiIdx);
  }

  return result;
}

void Graph::validate() const
{
#if DAGOR_DBGLEVEL > 0
  NAU_ASSERTF_RETURN(nodes.size() == nodeStates.size(), , //
    "Inconsistent IR: node count %d and node state count %d should be equal!", nodes.size(), nodeStates.size());
  NAU_ASSERTF_RETURN(nodeNames.empty() || nodes.size() == nodeNames.size(), , //
    "Inconsistent IR: node count %d and node name count %d should be equal if names are present at all!", nodes.size(),
    nodeNames.size());
  NAU_ASSERTF_RETURN(resourceNames.empty() || resources.size() == resourceNames.size(), , //
    "Inconsistent IR: resource count %d and resource name count %d should be equal if names are present at all!", resources.size(),
    resourceNames.size());

  for (auto &node : nodes)
  {
    for (const auto pred : node.predecessors)
      NAU_ASSERTF(pred < nodes.size(), "Inconsistent IR: missing node with index {}", pred);
    for (auto &req : node.resourceRequests)
      NAU_ASSERTF(req.resource < resources.size(), "Inconsistent IR: missing resource with index {}", req.resource);
  }

  for (auto [nodeIdx, state] : nodeStates.enumerate())
  {
    const auto validateRes = [this, nodeIdx = nodeIdx](ResourceIndex res_idx) {
      NAU_ASSERTF_RETURN(res_idx < resources.size(), , "Inconsistent IR: missing resource with index {}", res_idx);

      bool found = false;
      for (const auto &req : nodes[nodeIdx].resourceRequests)
        if (req.resource == res_idx)
          found = true;
      NAU_ASSERTF(found, "Inconsistent IR: resource {} used in node {} state, but was not requested by it!", resourceNames[res_idx],
        nodeNames[nodeIdx]);
    };

    if (state.vrs.has_value())
      validateRes(state.vrs->rateTexture);

    for (const auto &[_, binding] : state.bindings)
      if (binding.resource.has_value())
        validateRes(*binding.resource);

    if (state.pass.has_value())
    {
      if (state.pass->depthAttachment.has_value())
        validateRes(state.pass->depthAttachment->resource);
      for (const auto &color : state.pass->colorAttachments)
        if (color.has_value())
          validateRes(color->resource);
    }
  }
#endif
}

Mapping::Mapping(uint32_t node_count, uint32_t res_count, uint32_t multiplexing_extent) :
  mappedNodeNameIdCount_{node_count},
  mappedResNameIdCount_{res_count},
  nodeNameIdMapping_(node_count * multiplexing_extent, NODE_NOT_MAPPED),
  resNameIdMapping_(res_count * multiplexing_extent, RESOURCE_NOT_MAPPED)
{}

bool Mapping::wasResMapped(ResNameId id, MultiplexingIndex multi_idx) const
{
    return nau::to_underlying(multi_idx) * mappedResNameIdCount_ < resNameIdMapping_.size() &&
           nau::to_underlying(id) < mappedResNameIdCount_ && mapRes(id, multi_idx) != RESOURCE_NOT_MAPPED;
}

bool Mapping::wasNodeMapped(NodeNameId id, MultiplexingIndex multi_idx) const
{
    return nau::to_underlying(multi_idx) * mappedNodeNameIdCount_ < nodeNameIdMapping_.size() &&
           nau::to_underlying(id) < mappedNodeNameIdCount_ && mapNode(id, multi_idx) != NODE_NOT_MAPPED;
}

uint32_t Mapping::multiplexingExtent() const
{
  return mappedNodeNameIdCount_ ? nodeNameIdMapping_.size() / mappedNodeNameIdCount_ : 0;
}

ResourceIndex &Mapping::mapRes(ResNameId id, MultiplexingIndex multi_idx)
{
    return resNameIdMapping_[nau::to_underlying(multi_idx) * mappedResNameIdCount_ + nau::to_underlying(id)];
}

// Const cast is ok in below cases as we never mutate `this`, only copy a
// value from there. This trick avoids copy-pasta.
ResourceIndex Mapping::mapRes(ResNameId id, MultiplexingIndex multi_idx) const
{
  return const_cast<Mapping *>(this)->mapRes(id, multi_idx);
}

NodeIndex &Mapping::mapNode(NodeNameId id, MultiplexingIndex multi_idx)
{
    return nodeNameIdMapping_[nau::to_underlying(multi_idx) * mappedNodeNameIdCount_ + nau::to_underlying(id)];
}

NodeIndex Mapping::mapNode(NodeNameId id, MultiplexingIndex multi_idx) const
{
  return const_cast<Mapping *>(this)->mapNode(id, multi_idx);
}

} // namespace intermediate

} // namespace dabfg
