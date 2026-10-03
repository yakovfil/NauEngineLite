// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <stdint.h>

#include <functional>
#include <limits>

namespace net
{

typedef uint16_t sequence_t;

template <typename T>
inline constexpr bool is_seq_gt(T s1, T s2) // (s1 > s2)?
{
    const T halfMax = std::numeric_limits<T>::max() / 2;
    return ((s1 > s2) && (s1 - s2 <= halfMax)) || ((s2 > s1) && (s2 - s1 > halfMax));
}

template <typename T>
inline constexpr bool is_seq_lt(T s1, T s2) // (s1 < s2)?
{
  return is_seq_gt(s2, s1);
}

template <typename T>
inline T seq_diff(T big, T small)
{
  if (big >= small)
    return big - small;
  else // overflow
      return (std::numeric_limits<sequence_t>::max() - small) + big + 1;
}

struct LessSeq
{
  constexpr bool operator()(sequence_t a, sequence_t b) const { return is_seq_lt(a, b); }
};


} // namespace net
