// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <optional>
#include <span>
#include <tuple>
#include <variant>
#include <vector>

#include "dabfg/common/autoResolutionData.h"
#include "dabfg/common/bindingType.h"
#include "dabfg/id/idIndexedMapping.h"
#include "dabfg/id/idSentinels.h"
#include "nau/3d/dag_drv3d.h"
#include "nau/3d/dag_resPtr.h"
#include "nau/shaders/dag_shaderCommon.h"
#include "nau/utils/dag_fixedVectorSet.h"
#include "nau/utils/dag_relocatableFixedVector.h"
#include "nau/utils/fixed_map.h"
#include "nau/utils/inline_string.h"
#include "render/daBfg/detail/access.h"
#include "render/daBfg/detail/blob.h"
#include "render/daBfg/detail/nodeNameId.h"
#include "render/daBfg/detail/projectors.h"
#include "render/daBfg/detail/resNameId.h"
#include "render/daBfg/externalResources.h"
#include "render/daBfg/history.h"
#include "render/daBfg/multiplexing.h"
#include "render/daBfg/priority.h"
#include "render/daBfg/stage.h"
#include "render/daBfg/usage.h"

// This files contains an intermediate representation of the graph which
// decouples internal FG algorithms user-facing classes.
// The main aim of these structures is to describe how the user graph
// should look, possibly after applying transformations.

namespace dabfg::intermediate
{

// Denotes an index inside GraphIR::nodes
enum NodeIndex : uint16_t
{
};

// Denotes an index inside GraphIR::resources
enum ResourceIndex : uint16_t
{
};

// Opaque index denoting the iteration of multiplexing that this entity
// corresponds to. Should only be used for determining the index to
// provide to frontend, not for actual logic.
enum MultiplexingIndex : uint16_t
{
};

struct ResourceUsage
{
  Access access : 2;
  Usage type : 6;
  Stage stage : 6;
};
static_assert(sizeof(ResourceUsage) == 2);

struct Request
{
  ResourceIndex resource;
  ResourceUsage usage;
  // Marks that a node wants to use the last frame version of this resource
  bool fromLastFrame;
};

struct Node
{
  // TODO: this approach won't work with subpasses
  priority_t priority;

  // Resources that are used by this node
  dag::RelocatableFixedVector<Request, 16> resourceRequests;

  // Nodes that have to be executed before this one
  dag::FixedVectorSet<NodeIndex, 16> predecessors;

  NodeNameId frontendNode;

  // Multiplexing iteration this node will be executed on
  MultiplexingIndex multiplexingIndex;
};

struct ScheduledResource
{
    std::variant<ResourceDescription, BlobDescription> description;
    ResourceType resourceType;
    std::optional<AutoResolutionData> resolutionType;
    History history;

    bool isGpuResource() const
    {
        return std::holds_alternative<ResourceDescription>(description);
    }
    bool isCpuResource() const
    {
        return std::holds_alternative<BlobDescription>(description);
    }

  const ResourceDescription &getGpuDescription() const
  {
    NAU_ASSERT(isGpuResource());
    return std::get<ResourceDescription>(description);
  }
  const BlobDescription &getCpuDescription() const
  {
    NAU_ASSERT(isCpuResource());
    return std::get<BlobDescription>(description);
  }
};

struct Resource
{
    std::variant<ScheduledResource, ExternalResource> resource;

    // Renaming a resource changes its' ResNameId, but still leaves it
    // the same resource conceptually, so we map all such resources to same
    // IR resource.
    dag::FixedVectorSet<ResNameId, 8> frontendResources;

    // Multiplexing iteration this resource is produced on
    MultiplexingIndex multiplexingIndex;

    bool isExternal() const
    {
        return std::holds_alternative<ExternalResource>(resource);
    }
    bool isScheduled() const
    {
        return std::holds_alternative<ScheduledResource>(resource);
    }
  bool isExternalTex() const
  {
      return isExternal() && std::holds_alternative<ManagedTexView>(std::get<ExternalResource>(resource));
  }
  ManagedTexView getExternalTex() const
  {
    NAU_ASSERT(isExternalTex());
    return std::get<ManagedTexView>(std::get<ExternalResource>(resource));
  }
  bool isExternalBuf() const
  {
      return isExternal() && std::holds_alternative<ManagedBufView>(std::get<ExternalResource>(resource));
  }
  ManagedBufView getExternalBuf() const
  {
    NAU_ASSERT(isExternalBuf());
    return std::get<ManagedBufView>(std::get<ExternalResource>(resource));
  }
  ScheduledResource &asScheduled()
  {
    NAU_ASSERT(isScheduled());
    return std::get<ScheduledResource>(resource);
  }
  const ScheduledResource &asScheduled() const
  {
    NAU_ASSERT(isScheduled());
    return std::get<ScheduledResource>(resource);
  }
  ResourceType getResType() const
  {
    return isScheduled()     ? asScheduled().resourceType
           : isExternalTex() ? ResourceType::Texture
           : isExternalBuf() ? ResourceType::Buffer
                             : ResourceType::Invalid;
  }
};

// Default value here is "unbind everything"
struct Binding
{
  BindingType type = BindingType::Invalid;
  std::optional<ResourceIndex> resource = std::nullopt;
  bool history = false;

  // projected type of "what"
  ResourceSubtypeTag projectedTag = ResourceSubtypeTag::Invalid;
  // projector to get the value
  detail::TypeErasedProjector projector = +[](const void *data) { return data; };
};

//using BindingsMap = dag::FixedVectorMap<int, Binding, 8>;
using BindingsMap = nau::FixedMap<int, Binding, 8>;

struct VrsState
{
  uint32_t rateX;
  uint32_t rateY;
  ResourceIndex rateTexture;
  VariableRateShadingCombiner vertexCombiner;
  VariableRateShadingCombiner pixelCombiner;
};

struct SubresourceRef
{
  ResourceIndex resource;
  uint32_t mipLevel;
  uint32_t layer;
};

// No subresource ref means we want an empty binding for that attachment
struct RenderPass
{
    dag::RelocatableFixedVector<std::optional<SubresourceRef>, 8> colorAttachments;
    std::optional<SubresourceRef> depthAttachment;
    bool depthReadOnly = true;
};

struct ShaderBlockBindings
{
  int objectLayer = -1;
  int frameLayer = -1;
  int sceneLayer = -1;
};

struct RequiredNodeState
{
  bool wire = false;
  std::optional<VrsState> vrs;
  // NOTE: we can be within a render pass even with no attachments bound
  std::optional<RenderPass> pass;
  std::optional<shaders::OverrideState> shaderOverrides;
  BindingsMap bindings;
  ShaderBlockBindings shaderBlockLayers;
};

class Mapping;

using DebugNodeName = nau::InlineString<char, 64>;
// Resource names are longer due to renaming chains
using DebugResourceName = nau::InlineString<char, 128>;

struct Graph
{
  IdIndexedMapping<ResourceIndex, Resource> resources;
  IdIndexedMapping<NodeIndex, Node> nodes;
  // DoD suggests these should not be inside Node/Resource
  IdIndexedMapping<NodeIndex, RequiredNodeState> nodeStates;
  IdIndexedMapping<NodeIndex, DebugNodeName> nodeNames;
  IdIndexedMapping<ResourceIndex, DebugResourceName> resourceNames;

  // Choses a subgraph by selecting some nodes and specifying their order
  // Argument must map "old -> new" index and not contain any gaps
  void choseSubgraph(std::span<const intermediate::NodeIndex> old_to_new_index_mapping);

  // Calculates the original resources/nodes -> IR resources/nodes mapping
  Mapping calculateMapping();

  // NAU_LOG_ERRORs in debug build if state is inconsistent
  // only useful for finding bugs in IR generation
  void validate() const;
};


// These support structures can be used to determine which IR entity
// a frontend resource or a node got mapped to (and whether they got mapped at all)

inline constexpr ResourceIndex RESOURCE_NOT_MAPPED = MINUS_ONE_SENTINEL_FOR<ResourceIndex>;
inline constexpr NodeIndex NODE_NOT_MAPPED = MINUS_ONE_SENTINEL_FOR<NodeIndex>;

class Mapping final
{
public:
  Mapping() = default;
  Mapping(uint32_t node_count, uint32_t res_count, uint32_t multiplexing_extent);

  bool wasResMapped(ResNameId id, MultiplexingIndex multiIdx) const;
  bool wasNodeMapped(NodeNameId id, MultiplexingIndex multiIdx) const;

  uint32_t multiplexingExtent() const;
  uint32_t mappedNodeNameIdCount() const { return mappedNodeNameIdCount_; }
  uint32_t mappedResNameIdCount() const { return mappedResNameIdCount_; }
  uint32_t mappedNodeIndexCount() const { return nodeNameIdMapping_.size(); }
  uint32_t mappedResourceIndexCount() const { return resNameIdMapping_.size(); }

  ResourceIndex &mapRes(ResNameId id, MultiplexingIndex multiIdx);
  ResourceIndex mapRes(ResNameId id, MultiplexingIndex multiIdx) const;
  NodeIndex &mapNode(NodeNameId id, MultiplexingIndex multiIdx);
  NodeIndex mapNode(NodeNameId id, MultiplexingIndex multiIdx) const;

private:
  uint32_t mappedNodeNameIdCount_{0};
  uint32_t mappedResNameIdCount_{0};

  std::vector<NodeIndex> nodeNameIdMapping_{};
  std::vector<ResourceIndex> resNameIdMapping_{};
};

} // namespace dabfg::intermediate

DAG_DECLARE_RELOCATABLE(dabfg::intermediate::Binding);
DAG_DECLARE_RELOCATABLE(dabfg::intermediate::Request);
DAG_DECLARE_RELOCATABLE(std::optional<dabfg::intermediate::SubresourceRef>);
