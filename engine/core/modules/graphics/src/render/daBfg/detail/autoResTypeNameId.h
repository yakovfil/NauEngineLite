// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <stdint.h>

#include <type_traits>

namespace dabfg
{

enum class AutoResTypeNameId : uint16_t
{
    Invalid = static_cast<std::underlying_type_t<AutoResTypeNameId>>(-1)
};


} // namespace dabfg
