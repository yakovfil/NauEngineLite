// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <stdint.h>

#include <type_traits>

// TODO: change to dabfg::detail
namespace dabfg
{

enum class ResNameId : uint16_t
{
    Invalid = static_cast<std::underlying_type_t<ResNameId>>(-1)
};

} // namespace dabfg
