// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once
#include <type_traits>

namespace nau
{
    template <typename Enum>
    requires std::is_enum_v<Enum>
    constexpr std::underlying_type_t<Enum> to_underlying(Enum value) noexcept
    {
        return static_cast<std::underlying_type_t<Enum>>(value);
    }
}  // namespace nau
