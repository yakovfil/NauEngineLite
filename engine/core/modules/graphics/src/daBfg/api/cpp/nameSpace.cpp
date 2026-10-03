// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include "render/daBfg/nameSpace.h"

#include <span>

#include "dabfg/runtime/runtime.h"
#include "nau/diag/logging.h"
#include "nau/math/math.h"

namespace dabfg
{

NameSpace::NameSpace() : nameId{Runtime::get().getInternalRegistry().knownNames.root()}
{
}

NameSpace::NameSpace(NameSpaceNameId nid) : nameId(nid)
{
}

NameSpace NameSpace::operator/(const char *child_name) const
{
  return {Runtime::get().getInternalRegistry().knownNames.addNameId<NameSpaceNameId>(nameId, child_name)};
}

void NameSpace::setResolution(const char *type_name, nau::math::IVector2 value)
{
  auto &registry = Runtime::get().getInternalRegistry();

  const auto id = registry.knownNames.addNameId<AutoResTypeNameId>(nameId, type_name);
  const AutoResType newResolution = AutoResType{value, value};
  if (!registry.autoResTypes.isMapped(id) || registry.autoResTypes[id].staticResolution != newResolution.staticResolution ||
      registry.autoResTypes[id].dynamicResolution != newResolution.dynamicResolution)
  {
    Runtime::get().markStageDirty(CompilationStage::REQUIRES_RESOURCE_SCHEDULING);
    registry.autoResTypes.set(id, newResolution);
  }
}

void NameSpace::setDynamicResolution(const char *type_name, nau::math::IVector2 value)
{
  auto &registry = Runtime::get().getInternalRegistry();

  const auto id = registry.knownNames.addNameId<AutoResTypeNameId>(nameId, type_name);
  if (id == AutoResTypeNameId::Invalid || !registry.autoResTypes.isMapped(id))
  {
    NAU_LOG_ERROR("daBfg: Tried to set dynamic resolution for daBfg auto-res type {} that wasn't set yet!"
           " Please call NameSpace::setResolution(\"{}\", ...)!",
      type_name, type_name);
    return;
  }

  auto &autoResType = registry.autoResTypes[id];

  if (DAGOR_UNLIKELY(autoResType.staticResolution.getX() < value.getX() || autoResType.staticResolution.getY() < value.getY()))
  {
    NAU_LOG_ERROR("daBfg: Tried to set dynamic resolution '{}' to [{},{}], which is bigger than the static resolution [{},{}]!",
        type_name, value.getX(), value.getY(),
      autoResType.staticResolution.getX(), autoResType.staticResolution.getY());
    return;
  }

  if (DAGOR_UNLIKELY(value.getX() <= 0 || value.getY() <= 0))
  {
    NAU_LOG_ERROR("daBfg: Tried to set dynamic resolution '{}' to [{},{}], which is invalid!", type_name, value.getX(), value.getY());
    return;
  }

  // Noop, no need to update the countdown and run more useless code.
  if (autoResType.dynamicResolution == value)
    return;

  autoResType.dynamicResolution = value;
  // We can't immediately change resolution for all textures because history textures will lose
  // their data. So update counter here and decrease it by one when change resolution only for
  // current frame textures.
  autoResType.dynamicResolutionCountdown = ResourceScheduler::SCHEDULE_FRAME_WINDOW;
}

void NameSpace::fillSlot(NamedSlot slot, NameSpace res_name_space, const char *res_name)
{
  auto &registry = Runtime::get().getInternalRegistry();
  const ResNameId slotNameId = registry.knownNames.addNameId<ResNameId>(nameId, slot.name);
  const ResNameId resNameId = registry.knownNames.addNameId<ResNameId>(res_name_space.nameId, res_name);
  registry.resourceSlots.set(slotNameId, SlotData{resNameId});

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_NAME_RESOLUTION);
}

void NameSpace::updateExternallyConsumedResourceSet(std::span<const char* const> res_names)
{
  auto &registry = Runtime::get().getInternalRegistry();

  registry.sinkExternalResources.clear();
  for (auto name : res_names)
    registry.sinkExternalResources.insert(registry.knownNames.addNameId<ResNameId>(nameId, name));

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
}

void NameSpace::markResourceExternallyConsumed(const char *res_name)
{
  auto &registry = Runtime::get().getInternalRegistry();

  registry.sinkExternalResources.insert(registry.knownNames.addNameId<ResNameId>(nameId, res_name));

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
}

void NameSpace::unmarkResourceExternallyConsumed(const char *res_name)
{
  auto &registry = Runtime::get().getInternalRegistry();

  registry.sinkExternalResources.erase(registry.knownNames.addNameId<ResNameId>(nameId, res_name));

  Runtime::get().markStageDirty(CompilationStage::REQUIRES_IR_GRAPH_BUILD);
}


} // namespace dabfg
