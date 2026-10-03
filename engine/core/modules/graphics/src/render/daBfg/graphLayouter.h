// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "nau/utils/inline_vector.h"
#include "nau/utils/sorted_vector.h"

using NodeIdx = uint32_t;
using EdgeIdx = uint32_t;
using AdjacencyLists = std::vector<std::vector<NodeIdx>>;
using SortedAdjacencyLists = std::vector<nau::SortedVectorMultiSet<NodeIdx>>;
using NodeLayout = std::vector<std::vector<NodeIdx>>;

struct GraphLayout
{
  /**
   * Adjacency list updated with the fake node connectivity info.
   * Guarantee: for an edge u -> v in this graph, rank(u) + 1 = rank(v).
   */
  SortedAdjacencyLists adjacencyList;
  /**
   * A table of node indices that can be rendered as-is.
   * Has minimized edge-intersections and cluttering.
   * Each node index is contained here exactly once.
   * For `nodeLayout[i][j] = v`, `i` is called node `v`'s rank, while
   * `j` is called it's subrank.
   */
  NodeLayout nodeLayout;

  /**
   * Info useful for reconstructing additional edge data for the new
   * graph.
   * edgeMapping[i][j] = [(i', j1'), (i', j2'), ...]
   * where i -> adjacencyList[i][j] is an edge that came from
   * breaking up and merging the edges of the original graph:
   *   i' -> originalAdjacencyList[i'][j1'],
   *   i' -> originalAdjacencyList[i'][j2'], ...
   *
   * In layout without edges merging all arrays contain exactly one edge.
   */

  using OriginalEdges = nau::InlineVector<EdgeIdx, 8>;
  std::vector<std::vector<std::pair<NodeIdx, OriginalEdges>>> edgeMapping;
};

/**
 * Creates a human-readable layout (planarization) of a graph given by
 * it's adjacency lists representation.
 * Also requires a topological sort of the graph for now.
 *
 * This process introduces new fake nodes into the graph in order to
 * "break" edges. Fake nodes are supposed to be used as control points
 * when rendering edges with bezier curves or similar techniques.
 */
class IGraphLayouter
{
public:
  // TODO: make topsort an optional optimization
    virtual GraphLayout layout(const AdjacencyLists& graph, std::span<const NodeIdx> topsort, bool merge_fake_nodes) = 0;

    virtual ~IGraphLayouter() = default;
};

/**
 * Creates a default layouter based on graphviz's dot algorithm.
 */
std::unique_ptr<IGraphLayouter> make_default_layouter();
