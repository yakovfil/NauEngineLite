// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once

#include <type_traits>

namespace dag
{
    template <typename T, typename = void>
    struct is_type_relocatable : public std::bool_constant<std::is_trivially_copyable_v<T>>
    {
    };

    template <typename T>
    struct is_type_init_constructing : public std::true_type
    {
    };
}  // namespace dag

#define DAG_DECLARE_RELOCATABLE(C)                             \
    template <>                                                \
    struct dag::is_type_relocatable<C> : public std::true_type \
    {                                                          \
    }
