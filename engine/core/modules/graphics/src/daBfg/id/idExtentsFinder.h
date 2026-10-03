// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <algorithm>
#include <type_traits>

#include "nau/utils/enum_utils.h"

template <class EnumType>
class IdExtentsFinder
{
    using Underlying = std::underlying_type_t<EnumType>;

public:
    void update(EnumType value)
    {
        currentMax_ = std::max(currentMax_, nau::to_underlying(value));
    }

  Underlying get() const { return currentMax_ + 1; }

private:
  Underlying currentMax_{0};
};
