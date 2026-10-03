// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <span>

#include "dabfg/backend/intermediateRepresentation.h"

extern bool debug_graph_generation;


namespace dabfg
{
struct InternalRegistry;
class NameResolver;
struct DependencyData;
} // namespace dabfg

namespace dabfg
{
class NodeTracker;

void update_graph_visualization(InternalRegistry& registry, const DependencyData& deps, std::span<const NodeNameId> node_execution_order);
void invalidate_graph_visualization();
void reset_texture_visualization();

void update_resource_visualization(const InternalRegistry& registry, std::span<const NodeNameId> node_execution_order);
void debug_rec_resource_placement(ResNameId id, int frame, int heap, int offset, int size);
void debug_rec_resource_barrier(ResNameId res_id, int res_frame, int exec_time, int exec_frame, ResourceBarrier barrier);

void validation_restart();
void validation_set_current_node(const InternalRegistry &registry, NodeNameId node);
void validation_add_resource(const D3dResource *res);
void validation_of_external_resources_duplication(
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::Resource> &resources,
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &resourceNames);
void validate_global_state(const InternalRegistry &registry, NodeNameId node);

void dump_ir_graph(const intermediate::Graph &graph);
} // namespace dabfg
