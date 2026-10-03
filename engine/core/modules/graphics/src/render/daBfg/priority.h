// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <limits>

namespace dabfg
{

/// Type for daBfg node execution priorities
using priority_t = int32_t;

/// Default priority that is used when none is specified explicitly.
inline constexpr priority_t PRIO_DEFAULT = 0;

/// Max priority: the node will be executed as late as possible.
inline constexpr priority_t PRIO_AS_LATE_AS_POSSIBLE = std::numeric_limits<priority_t>::max();

/// Min priority: the node will be executed as early as possible.
inline constexpr priority_t PRIO_AS_EARLY_AS_POSSIBLE = std::numeric_limits<priority_t>::min();

} // namespace dabfg
