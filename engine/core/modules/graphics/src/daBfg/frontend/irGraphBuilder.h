// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <functional>
#include <span>
#include <utility>

#include "dabfg/backend/intermediateRepresentation.h"
#include "dabfg/id/idIndexedFlags.h"
#include "nau/memory/scoped_standard_allocator.h"
#include "nau/utils/dynamic_bit_vector.h"
#include "nau/utils/sorted_vector.h"

namespace dabfg
{

struct InternalRegistry;
struct DependencyData;
class NameResolver;

class IrGraphBuilder
{
public:
  IrGraphBuilder(const InternalRegistry &reg, const DependencyData &depDataCalc, const NameResolver &nameRes) :
    registry{reg}, depData{depDataCalc}, nameResolver{nameRes}
  {}

  // Returns an intermediate representation of the graph that
  // multiplexes nodes and resources (due to history resources,
  // stereo rendering, super/sub sampling, etc), groups nodes and
  // resources (due to subpasses and renaming modify), and culls out
  // unused or broken nodes/resources
  intermediate::Graph build(multiplexing::Extents extents) const;

private:
  const InternalRegistry &registry;
  const DependencyData &depData;
  const NameResolver &nameResolver;

  struct ValidityInfo
  {
      IdIndexedFlags<ResNameId, nau::FrameStdAllocator<nau::BitVectorWord>> resourceValid;
      IdIndexedFlags<NodeNameId, nau::FrameStdAllocator<nau::BitVectorWord>> nodeValid;
  };
  bool validateResource(ResNameId resId) const;
  bool validateNode(NodeNameId resId) const;
  void validateLifetimes(ValidityInfo &validity) const;
  ValidityInfo findValidResourcesAndNodes() const;

  // Result contains no edges yet
  std::pair<intermediate::Graph, intermediate::Mapping> createDiscreteGraph(const ValidityInfo& validity,
                                                                            multiplexing::Extents extents) const;
  void fixupFalseHistoryFlags(intermediate::Graph &graph) const;
  void setIrGraphDebugNames(intermediate::Graph &graph) const;
  // Actually adds the edges
  void addEdgesToIrGraph(intermediate::Graph &graph, const ValidityInfo &validity, const intermediate::Mapping &mapping) const;

  using SinkSet = nau::SortedVectorSet<intermediate::NodeIndex, std::less<intermediate::NodeIndex>, nau::FrameStdAllocator<intermediate::NodeIndex>>;
  SinkSet findSinkIrNodes(const intermediate::Graph& graph, const intermediate::Mapping& mapping, std::span<const ResNameId> sinkResources) const;

  // Returns a displacement (old index -> new index) partial mapping
  IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, nau::FrameStdAllocator<intermediate::NodeIndex>> pruneGraph(const intermediate::Graph& graph,
                                                                                                                                 const intermediate::Mapping& mapping,
                                                                                                                                 std::span<const intermediate::NodeIndex> sinkNodes,
                                                                                                                                 const ValidityInfo& validity) const;

  intermediate::RequiredNodeState calcNodeState(NodeNameId node_id, intermediate::MultiplexingIndex multi_index,
    const intermediate::Mapping &mapping) const;

  void dumpRawUserGraph(const ValidityInfo &info) const;
};

} // namespace dabfg
