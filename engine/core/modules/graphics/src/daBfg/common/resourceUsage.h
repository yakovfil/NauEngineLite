// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <optional>

#include "dabfg/backend/intermediateRepresentation.h"
#include "dabfg/frontend/internalRegistry.h"
#include "nau/3d/dag_drv3d.h"
#include "render/daBfg/detail/access.h"
#include "render/daBfg/detail/resourceType.h"
#include "render/daBfg/history.h"
#include "render/daBfg/stage.h"
#include "render/daBfg/usage.h"

namespace dabfg
{

void validate_usage(ResourceUsage usage);

ResourceBarrier barrier_for_transition(intermediate::ResourceUsage usage_before, intermediate::ResourceUsage usage_after);

std::optional<ResourceActivationAction> get_activation_from_usage(History history, intermediate::ResourceUsage usage, ResourceType res_type);
__forceinline std::optional<ResourceActivationAction> get_activation_from_usage(History history, ResourceUsage usage, ResourceType res_type)
{
  return get_activation_from_usage(history, intermediate::ResourceUsage{usage.access, usage.type, usage.stage}, res_type);
}

void update_creation_flags_from_usage(uint32_t &flags, ResourceUsage usage, ResourceType res_type);

} // namespace dabfg