// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <functional>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <string>
#include <utility>
#include <variant>

#include "dabfg/backend/intermediateRepresentation.h"
#include "dabfg/frontend/internalRegistry.h"
#include "dabfg/id/idRange.h"
#include "nau/diag/logging.h"
#include "nau/memory/scoped_standard_allocator.h"
#include "nau/utils/sorted_vector.h"
#include "render/daBfg/bfg.h"

// NOTE: this is defined inside scriptSElem.cpp
//extern void (*scripted_shader_element_on_before_resource_used)(const D3dResource *, const char *);

// NOTE: this is defined inside shaderBlock.cpp
//extern void (*shader_block_on_before_resource_used)(const D3dResource *, const char *);

namespace dabfg
{

    static nau::SortedVectorSet<const D3dResource*> managed_resources;
    static nau::SortedVectorSet<const D3dResource*> validated_external_resources;
    static nau::SortedVectorSet<const D3dResource*> current_node_resources;
    static std::string current_node_name;

    static nau::SortedVectorSet<std::pair<std::string, std::string>> found_errors;

    struct ResKeyHash
    {
        inline unsigned operator()(const D3DRESID& k) const
        {
            return std::hash<unsigned>{}(unsigned(k));
        }
};

static std::string get_res_owning_shadervars(const D3dResource* res)
{
    std::string message = "Shadervars bound to illegally accessed FG resource:";
    // for (int varId = 0, n = VariableMap::getGlobalVariablesCount(); varId < n; ++varId)
    //{
    //   int varType = ShaderGlobal::get_var_type(varId);
    //   if ((varType == SHVT_TEXTURE && D3dResManagerData::getD3dRes(ShaderGlobal::get_tex(varId)) == res) ||
    //       (varType == SHVT_BUFFER && D3dResManagerData::getD3dRes(ShaderGlobal::get_buf(varId)) == res))
    //     message.append_sprintf("\n  - '%s'", VariableMap::getVariableName(varId));
    // } // TODO: fix it
    return message;
}

static void validate(const D3dResource *res, const char *current_shader_class)
{
  if (managed_resources.find(res) == managed_resources.end() &&
      validated_external_resources.find(res) == validated_external_resources.end())
    return;

  if (current_node_name.empty())
  {
      if (found_errors.find({reinterpret_cast<const char*>(res->getResName()), "" }) == found_errors.end())
    {
      found_errors.emplace(reinterpret_cast<const char*>(res->getResName()), "");
      NAU_LOG_ERROR("Framegraph resource {} was used by {} outside of a node's execute method! "
             "This should never happen, make sure that you are not caching "
             "references to FG resources somewhere.\n{}",
        res->getResName(), current_shader_class, get_res_owning_shadervars(res));
    }
    return;
  }

  if (current_node_resources.find(res) == current_node_resources.end() &&
      found_errors.find({reinterpret_cast<const char*>(res->getResName()), current_node_name}) == found_errors.end())
  {
    found_errors.emplace(reinterpret_cast<const char*>(res->getResName()), current_node_name);
    NAU_LOG_ERROR("Framegraph resource {} was used by {} in node {} either without being "
           "requested by the node at all, or without being provided by "
           "framegraph afterwards.\n{}",
      res->getResName(), current_shader_class, current_node_name, get_res_owning_shadervars(res));
  }
}

void validation_restart()
{
  //scripted_shader_element_on_before_resource_used = &validate; // TODO: fix it
  //shader_block_on_before_resource_used = &validate;
  managed_resources.clear();
}

void validation_set_current_node(const InternalRegistry &registry, NodeNameId nodeId)
{
  current_node_resources.clear();
  current_node_name.clear();

  if (nodeId == NodeNameId::Invalid)
    return;

  current_node_name = registry.knownNames.getName(nodeId);

  current_node_resources.reserve(
    registry.resourceProviderReference.providedResources.size() + registry.resourceProviderReference.providedHistoryResources.size());

  auto addResource = [](const auto& var)
  {
      if (auto tex = std::get_if<ManagedTexView>(&var); tex != nullptr && *tex)
      {
          current_node_resources.emplace(static_cast<const D3dResource*>(tex->getBaseTex()));
      }
      else if (auto buf = std::get_if<ManagedBufView>(&var); buf != nullptr && *buf)
      {
          current_node_resources.emplace(static_cast<const D3dResource*>(buf->getBuf()));
      }
  };

  for (const auto &[id, res] : registry.resourceProviderReference.providedResources)
    addResource(res);

  for (const auto &[id, res] : registry.resourceProviderReference.providedHistoryResources)
    addResource(res);
}

void validation_add_resource(const D3dResource *res) { managed_resources.emplace(res); }

void validation_of_external_resources_duplication(
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::Resource> &resources,
  const IdIndexedMapping<intermediate::ResourceIndex, intermediate::DebugResourceName> &resourceNames)
{
  static ska::flat_hash_set<D3DRESID, ResKeyHash> alreadyLoggedResources;

  //FRAMEMEM_VALIDATE; // Reserved amount DEFINITELY should be enough
  ska::flat_hash_map<D3DRESID, intermediate::ResourceIndex, ResKeyHash, std::equal_to<D3DRESID>, nau::FrameStdAllocator<std::pair<const D3DRESID, intermediate::ResourceIndex>>>
      setOfExternalResources;
  setOfExternalResources.reserve(resources.size());

  for (const auto resIdx : IdRange<intermediate::ResourceIndex>(resources.size()))
  {
    if (!resources[resIdx].isExternal())
      continue;

    const auto resType = resources[resIdx].getResType();

    D3DRESID resId;
    if (resType == ResourceType::Texture)
      resId = resources[resIdx].getExternalTex().getTexId();
    else if (resType == ResourceType::Buffer)
      resId = resources[resIdx].getExternalBuf().getBufId();
    else
      NAU_ASSERT(false);

    if (auto [it, succ] = setOfExternalResources.emplace(resId, resIdx); !succ)
      // TODO: When all duplications will be fixed change logwarn to NAU_LOG_ERROR. Now we can't becuse have some duplications in master.
      if (alreadyLoggedResources.insert(resId).second)
        NAU_LOG_WARNING("The physical resource '{}' was registered as an external resource into frame graph"
                " for two different intermediate resources: '{}' and '{}'. This is not allowed and"
                " will lead to broken barriers and resource states!",
          ::get_managed_res_name(resId), resourceNames[resIdx], resourceNames[it->second]);
  }
}

} // namespace dabfg

namespace dabfg
{
void mark_external_resource_for_validation(const D3dResource *resource) { validated_external_resources.insert(resource); }
} // namespace dabfg