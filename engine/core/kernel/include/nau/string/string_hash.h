// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#pragma once
#include <functional>
#include <string_view>
namespace nau
{
    // Transient container lookup only; persistent identifiers keep their explicit hash algorithm.
    struct StringHash
    {
        using is_transparent = void;
        size_t operator()(std::string_view value) const noexcept
        {
            return std::hash<std::string_view>{}(value);
        }
    };
}  // namespace nau
