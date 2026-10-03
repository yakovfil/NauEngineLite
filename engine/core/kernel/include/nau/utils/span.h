// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once
#include <cstdint>
#include <span>

namespace nau
{
    template <typename T>
    using Span = std::span<T>;  // Alias

    template <typename T>
    using ConstSpan = nau::Span<T const>;  // Alias

    template <class V, typename = typename V::allocator_type>
    inline void clear_and_shrink(V& v)
    {
        v = V(v.get_allocator());
    }

}  // namespace nau

template <class T>
inline nau::Span<T> make_span(T* p, intptr_t n)
{
    return nau::Span<T>(p, n);
}

// TODO: Remove when ecs code generetion is rewritten.
template <typename T, size_t Size>
inline nau::Span<T> make_span(T (&_arr)[Size])
{
    return {_arr};
}

template <typename Container, typename T = typename Container::value_type>
inline nau::Span<T> make_span(Container& container)
{
    return {container.data(), container.size()};
}

template <class T>
inline nau::ConstSpan<T> make_span_const(const T* p, intptr_t n)
{
    return nau::ConstSpan<T>(p, n);
}
template <class T, size_t N>
inline nau::ConstSpan<T> make_span_const(const T (&a)[N])
{
    return nau::ConstSpan<T>(a, N);
}

template <typename Container, typename T = typename Container::value_type>
inline nau::ConstSpan<T> make_span_const(
    const Container& container)
{
    return {container.data(), container.size()};
}
