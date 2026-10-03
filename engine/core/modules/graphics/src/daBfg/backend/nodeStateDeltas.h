// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <optional>
#include <variant>

#include "dabfg/backend/intermediateRepresentation.h"

// sd stands for state deltas
namespace dabfg::sd
{

struct ShaderBlockLayersInfoDelta
{
    std::optional<int> objectLayer;
    std::optional<int> frameLayer;
    std::optional<int> sceneLayer;
};

// This more akin to "commands" that tell the executor that some state
// needs to be updated in the driver. `nullopt` means that that
// particular piece of state does not need updating.

struct StopVrs
{};
using VrsStateDelta = std::variant<StopVrs, intermediate::VrsState>;
struct FinishRenderPass
{};
using RenderPassDelta = std::variant<FinishRenderPass, intermediate::RenderPass>;

struct NodeStateDelta
{
    std::optional<bool> wire;
    std::optional<VrsStateDelta> vrs;
    std::optional<RenderPassDelta> pass;
    std::optional<shaders::UniqueOverrideStateId> shaderOverrides;
    intermediate::BindingsMap bindings;
    ShaderBlockLayersInfoDelta shaderBlockLayers;
};

using NodeStateDeltas = IdIndexedMapping<intermediate::NodeIndex, NodeStateDelta>;

NodeStateDeltas calculate_per_node_state_deltas(const intermediate::Graph &graph);

} // namespace dabfg::sd
