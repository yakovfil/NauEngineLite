// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <type_traits>

namespace dabfg
{

    template <class T, class = std::enable_if_t<std::is_enum_v<T> && std::is_unsigned_v<std::underlying_type_t<T>>>>
    constexpr T MINUS_ONE_SENTINEL_FOR = static_cast<T>(static_cast<std::underlying_type_t<T>>(-1));

    template <class T, class = std::enable_if_t<std::is_enum_v<T> && std::is_unsigned_v<std::underlying_type_t<T>>>>
    constexpr T MINUS_TWO_SENTINEL_FOR = static_cast<T>(static_cast<std::underlying_type_t<T>>(-2));

} // namespace dabfg
