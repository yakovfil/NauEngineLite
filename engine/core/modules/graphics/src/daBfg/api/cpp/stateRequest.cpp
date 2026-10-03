// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "render/daBfg/stateRequest.h"

#include <utility>

#include "dabfg/frontend/internalRegistry.h"
#include "nau/diag/logging.h"

namespace dabfg
{

StateRequest::StateRequest(InternalRegistry *reg, NodeNameId nodeId) : id{nodeId}, registry{reg}
{
  auto &reqs = registry->nodes[id].stateRequirements;
  if (NAU_UNLIKELY(reqs.has_value()))
  {
    NAU_LOG_ERROR("Global state requested twice on '{}' frame graph node!"
           " Ignoring one of the requests!",
      registry->knownNames.getName(nodeId));
  }
  reqs.emplace();
}

StateRequest StateRequest::setFrameBlock(const char *block) &&
{
    int previousValue = std::exchange(registry->nodes[id].shaderBlockLayers.frameLayer,
                                      0  // ShaderGlobal::getBlockId(block) // TODO: fix it
    );

    if (NAU_UNLIKELY(previousValue != -1))
    {
        NAU_LOG_ERROR(
            "Block requested to be set to layer 'FRAME' twice within"
            " '%s' frame graph node! Ignoring one of the requests!",
            registry->knownNames.getName(id));
    }

  return *this;
}

StateRequest StateRequest::setSceneBlock(const char *block) &&
{
    int previousValue = std::exchange(registry->nodes[id].shaderBlockLayers.sceneLayer,
                                      0  // ShaderGlobal::getBlockId(block)
    );

    if (NAU_UNLIKELY(previousValue != -1))
    {
        NAU_LOG_ERROR(
            "Block requested to be set to layer 'SCENE' twice within"
            " '%s' frame graph node! Ignoring one of the requests!",
            registry->knownNames.getName(id));
    }

  return *this;
}

StateRequest StateRequest::setObjectBlock(const char *block) &&
{
    int previousValue = std::exchange(registry->nodes[id].shaderBlockLayers.objectLayer,
                                      0  // ShaderGlobal::getBlockId(block)
    );

    if (NAU_UNLIKELY(previousValue != -1))
    {
        NAU_LOG_ERROR(
            "Block requested to be set to layer 'OBJECT' twice within"
            " '%s' frame graph node! Ignoring one of the requests!",
            registry->knownNames.getName(id));
    }

  return *this;
}

StateRequest StateRequest::allowWireframe() &&
{
  registry->nodes[id].stateRequirements->supportsWireframe = true;
  return *this;
}

StateRequest StateRequest::allowVrs(VrsRequirements vrs) &&
{
  if (!vrs.rateTexture.has_value())
  {
    registry->nodes[id].stateRequirements->vrsState.reset();
    return *this;
  }

  const auto rateTexId = vrs.rateTexture->resUid.resId;
  std::move(*vrs.rateTexture).texture().atStage(dabfg::Stage::ALL_GRAPHICS).useAs(dabfg::Usage::VRS_RATE_TEXTURE);
  registry->nodes[id].stateRequirements->vrsState =
    VrsStateRequirements{vrs.rateX, vrs.rateY, rateTexId, vrs.vertexCombiner, vrs.pixelCombiner};
  return *this;
}

StateRequest StateRequest::enableOverride(shaders::OverrideState override) &&
{
  registry->nodes[id].stateRequirements->pipelineStateOverride = override;
  return *this;
}

} // namespace dabfg
