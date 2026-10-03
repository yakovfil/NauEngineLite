// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <ska_hash_map/flat_hash_map2.hpp>
#include <span>

#include "dabfg/backend/intermediateRepresentation.h"
#include "dabfg/common/graphDumper.h"
#include "dabfg/id/idIndexedFlags.h"
#include "nau/memory/scoped_standard_allocator.h"

namespace dabfg
{

class NodeScheduler
{
public:
  NodeScheduler(IGraphDumper &dumper) : graphDumper{dumper} {}

  // old index -> new index mapping
  using NodePermutation = IdIndexedMapping<intermediate::NodeIndex, intermediate::NodeIndex, nau::FrameStdAllocator<intermediate::NodeIndex>>;
  NodePermutation schedule(const intermediate::Graph &graph);

private:
  IGraphDumper &graphDumper;
};

} // namespace dabfg
