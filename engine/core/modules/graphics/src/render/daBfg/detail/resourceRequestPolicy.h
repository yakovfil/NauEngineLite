// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <utility>

#include "nau/utils/enum_utils.h"

namespace dabfg::detail
{
// Determines which functionality is included in the request class and
// avoid logic stupid errors at compile time.
enum class ResourceRequestPolicy : uint32_t
{
  None = 0,
  Readonly = 0b00000001,
  Optional = 0b00000010,
  History = 0b00000100,
  HasUsageStage = 0b00001000,
  HasUsageType = 0b00010000,
};

inline constexpr ResourceRequestPolicy operator&(ResourceRequestPolicy fst, ResourceRequestPolicy snd)
{
    return static_cast<ResourceRequestPolicy>(nau::to_underlying(fst) & nau::to_underlying(snd));
}

inline constexpr ResourceRequestPolicy operator|(ResourceRequestPolicy fst, ResourceRequestPolicy snd)
{
    return static_cast<ResourceRequestPolicy>(nau::to_underlying(fst) | nau::to_underlying(snd));
}

} // namespace dabfg::detail
