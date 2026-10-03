// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <algorithm>
#include <iterator>

#include "constants.h"

namespace drv3d_dx12
{

struct ConstRegisterType
{
  uint32_t components[SHADER_REGISTER_ELEMENTS];
};
inline bool operator==(const ConstRegisterType &l, const ConstRegisterType &r)
{
    return std::equal(std::begin(l.components), std::end(l.components), std::begin(r.components));
}

} // namespace drv3d_dx12
