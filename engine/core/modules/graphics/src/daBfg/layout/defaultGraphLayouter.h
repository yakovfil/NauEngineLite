// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <span>
#include <utility>
#include <vector>

#include "render/daBfg/graphLayouter.h"

class DefaultGraphLayouter : public IGraphLayouter
{
  using rank_t = size_t;

public:
  // Algorithm losely based on
  // A Technique for Drawing Directed Graphs
  // by Emden R. Gansner, Eleftherios Koutsofios, Stephen C. North, and Gem-Phong Vo
    GraphLayout layout(const AdjacencyLists& graph, std::span<const NodeIdx> topsort, bool merge_fake_nodes) override;

private:
  // Initializes nodeRanks
    void rankNodes(const AdjacencyLists& graph, std::span<const NodeIdx> topsort);

    // Initializes nodeTable and nodeSubranks
    void generateInitialLayout(std::span<const NodeIdx> topsort);

    // Initializes updatedGraph and inverseUpdatedGraph
    void breakMultirankEdges(const AdjacencyLists& graph, bool merge_fake_nodes);

    size_t calculateCrossings(rank_t rank);
    size_t calculateTotalCrossings();
    void transposeNodes(rank_t rank, rank_t firstSubrank, rank_t secondSubrank);

    std::vector<std::pair<float, uint32_t>> calculateWavgHeuristic(uint32_t rank, bool forward);

    void optimizeHeuristicSort(bool ascend);
    void optimizeTranspose();

    void optimizeLayout(size_t iterations);

private:
  SortedAdjacencyLists updatedGraph;
  SortedAdjacencyLists inverseUpdatedGraph;

  std::vector<rank_t> nodeRanks;
  std::vector<rank_t> nodeSubranks;
  NodeLayout nodeTable;

  std::vector<std::vector<std::pair<NodeIdx, GraphLayout::OriginalEdges>>> edgeMapping;
};
