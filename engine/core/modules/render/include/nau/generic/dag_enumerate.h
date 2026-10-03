// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#ifndef _DAGOR_PUBLIC_GENERIC_DAG_ENUMERATE_H_
#define _DAGOR_PUBLIC_GENERIC_DAG_ENUMERATE_H_
#pragma once

#include <iterator>
#include <tuple>
#include <utility>

// simple helper mimicking python's enumerate() function
// useful for keeping a counter when iterating in a range-based for loop
// for(auto [i,v] : enumerate(container)) { ... }
template <typename T>
constexpr auto enumerate(T &&container, size_t start_index = 0)
{
  struct EnumerationHelper
  {
      using IteratorType = decltype(std::begin(std::declval<T>()));

      struct iterator
      {
          constexpr iterator operator++()
          {
              ++it;
              ++counter;
              return *this;
          }

          constexpr bool operator!=(iterator other)
          {
              return it != other.it;
          }

          constexpr auto operator*()
          {
              return std::tuple<size_t, decltype(*it)>{counter, *it};
          }

          IteratorType it = {};
          size_t counter = 0;
      };

      constexpr iterator begin()
      {
          return {std::begin(container), startIndex};
      }

      constexpr iterator end()
      {
          return {std::end(container)};
      }

    T container;
    size_t startIndex;
  };

  return EnumerationHelper{std::forward<T>(container), start_index};
}

#endif //_DAGOR_PUBLIC_GENERIC_DAG_ENUMERATE_H_