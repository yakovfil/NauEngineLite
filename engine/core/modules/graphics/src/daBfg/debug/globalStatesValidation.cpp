// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <optional>
#include <string_view>
#include <variant>

#include "dabfg/frontend/internalRegistry.h"
#include "dabfg/frontend/nameResolver.h"
#include "nau/diag/logging.h"
#include "nau/shaders/dag_shaderVarType.h"

namespace dabfg
{

constexpr auto shaderVarValidationMessage = "Frame graph node %s expected the shader variable %s to be bound to the %s %s, "
                                            "but it was spuriously changed to %s while executing the node! Please remove the spurious "
                                            "set, or reset it back to the expected value at the end of the function execution!";

constexpr auto shaderBlockValidationMessage =
  "Frame graph node %s expected the %s layer to contain the shader block %s, "
  "but it was spuriously changed to %s while executing the node! Please remove the spurious set, "
  "or reset it back to the expected value at the end of the function execution!";

void validate_global_state(const InternalRegistry &registry, NodeNameId nodeId)
{
  //TIME_PROFILE(fg_validate_global_state);

  const auto &nodeData = registry.nodes[nodeId];
  for (const auto &[shVar, res] : nodeData.bindings)
  {
    if (res.type != BindingType::ShaderVar)
      continue;

    D3DRESID expectedId;

    // Shaders dump can be reloaded and some shader vars can be absent in new dump
    //if (!VariableMap::isGlobVariablePresent(shVar)) // TODO: we need something as replacement for VariableMap
    //  continue;

    {
      const auto &provided = res.history ? registry.resourceProviderReference.providedHistoryResources
                                         : registry.resourceProviderReference.providedResources;
      const auto it = provided.find(res.resource);

      // If no resource was provided, this is an optional bind request
      // and we expect to see BAD_D3DRESID
      if (it != provided.end())
      {
          if (auto bufPtr = std::get_if<ManagedBufView>(&it->second))
          {
              expectedId = bufPtr->getBufId();
          }
          else if (auto texPtr = std::get_if<ManagedTexView>(&it->second))
          {
              expectedId = texPtr->getTexId();
          }
          else if (auto blobView = std::get_if<BlobView>(&it->second))
          {
              continue;
          }
      }
    }

    D3DRESID observedId;
    // SV type and resource type mismatch is impossible at this point
    std::string_view resTypeStr;
    //switch (ShaderGlobal::get_var_type(shVar))
    //{
    //  case SHVT_TEXTURE:
    //    resTypeStr = "texture";
    //    observedId = ShaderGlobal::get_tex(shVar);
    //    break;
    //
    //  case SHVT_BUFFER:
    //    resTypeStr = "buffer";
    //    observedId = ShaderGlobal::get_buf(shVar);
    //    break;
    //
    //  default:
    //    NAU_ASSERT(false); // impossible due to invariants
    //    continue;
    //}

    if (expectedId != observedId)
      NAU_LOG_ERROR(shaderVarValidationMessage,
          registry.knownNames.getName(nodeId),
          "VARIABLE_NAME",//VariableMap::getVariableName(shVar),
          resTypeStr,
          get_managed_res_name(expectedId),
          get_managed_res_name(observedId));
  }

  auto validateBlock = [&registry, nodeId](int node_block, int global_block, const char *layer) {
    if (node_block != global_block)
    {
      //const char *nodeBlockName = ShaderGlobal::getBlockName(node_block);
      //const char *globalBlockName = ShaderGlobal::getBlockName(global_block);
      //NAU_LOG_ERROR(shaderBlockValidationMessage, registry.knownNames.getName(nodeId), layer, nodeBlockName, globalBlockName);
    }
  };

  //validateBlock(nodeData.shaderBlockLayers.frameLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME), "frame");
  //validateBlock(nodeData.shaderBlockLayers.sceneLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE), "scene");
  //validateBlock(nodeData.shaderBlockLayers.objectLayer, ShaderGlobal::getBlock(ShaderGlobal::LAYER_OBJECT), "object");

  if (nodeData.renderingRequirements.has_value())
  {
    const auto &expectedRts = *nodeData.renderingRequirements;

    Driver3dRenderTarget observedRts;
    d3d::get_render_target(observedRts);

    const auto validateRt = [&registry, nodeId](const VirtualSubresourceRef& expected,
                                                const std::optional<Driver3dRenderTarget::RTState>& maybeObserved, const char* slot_name)
    {
        const auto& provided = registry.resourceProviderReference.providedResources;
        const auto it = provided.find(expected.nameId);

        BaseTexture* expectedTex = nullptr;
        if (it != provided.end())
            if (auto view = std::get_if<ManagedTexView>(&it->second))
                expectedTex = view->getTex2D();

        const auto expectedName = expectedTex ? reinterpret_cast<const char*>(expectedTex->getTexName()) : "NULL";
        const auto observedName = maybeObserved.has_value() && maybeObserved->tex ? reinterpret_cast<const char*>(maybeObserved->tex->getTexName()) : "NULL";

        // avoid the checks below for backbuffer
        if (maybeObserved.has_value() && maybeObserved->tex == nullptr)
            return;

        auto observed = maybeObserved.value_or(Driver3dRenderTarget::RTState{});

        if (expectedTex != observed.tex)
        {
            NAU_LOG_ERROR(
                "Frame graph node {} expected the render target in slot {} to be bound to {}, "
                "but it was spuriously changed to {} while executing the node! Please remove the spurious "
                "set, or reset it back to the expected value at the end of the function execution!",
                registry.knownNames.getName(nodeId), slot_name, expectedName, observedName);
            return;
        }

        if (expected.layer != observed.layer)
        {
            NAU_LOG_ERROR(
                "Frame graph node {} expected the render target in slot {} to be bound to the {} layer of texture {}, "
                "but it was spuriously changed to the {} layer while executing the node! Please remove the spurious "
                "set, or reset it back to the expected value at the end of the function execution!",
                registry.knownNames.getName(nodeId), slot_name, expected.layer, expectedName, observed.layer);
            return;
        }

        if (expected.mipLevel != observed.level)
        {
            NAU_LOG_ERROR(
                "Frame graph node {} expected the render target in slot {} to be bound to the {} mip level of texture {}, "
                "but it was spuriously changed to the {} mip level while executing the node! Please remove the spurious "
                "set, or reset it back to the expected value at the end of the function execution!",
                registry.knownNames.getName(nodeId), slot_name, expected.mipLevel, expectedName, observed.level);
            return;
        }
    };

    static constexpr const char *SLOT_NAMES[] = {
      "color 0", "color 1", "color 2", "color 3", "color 4", "color 5", "color 6", "color 7"};
    static_assert(countof(SLOT_NAMES) == Driver3dRenderTarget::MAX_SIMRT);
    for (uint32_t rtIdx = 0; rtIdx < Driver3dRenderTarget::MAX_SIMRT; ++rtIdx)
    {
      // We must validate that nothing was set into RT slots that were not requested
      VirtualSubresourceRef expectedRt = rtIdx < expectedRts.colorAttachments.size() ? expectedRts.colorAttachments[rtIdx]
                                                                                     : VirtualSubresourceRef{ResNameId::Invalid, 0, 0};
      std::optional<Driver3dRenderTarget::RTState> observedRt{};
      if (observedRts.isColorUsed(rtIdx))
        observedRt = observedRts.getColor(rtIdx);
      validateRt(expectedRt, observedRt, SLOT_NAMES[rtIdx]);
    }

    {
        std::optional<Driver3dRenderTarget::RTState> observedDepth{};
        if (observedRts.isDepthUsed())
            observedDepth = observedRts.getDepth();
        validateRt(expectedRts.depthAttachment, observedDepth, "depth");
    }

    if (expectedRts.depthAttachment.nameId != ResNameId::Invalid && expectedRts.depthReadOnly != observedRts.isDepthReadOnly())
    {
      NAU_LOG_ERROR("Frame graph node {} expected to have a {} depth, but it was spuriously changed "
             "to be the opposite. Please, remove the spurious depth target change from the node!",
        registry.knownNames.getName(nodeId), expectedRts.depthReadOnly ? "read-only" : "read-write");
    }
  }
}

} // namespace dabfg