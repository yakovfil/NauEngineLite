// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include "idEnumerateView.h"
#include "nau/utils/enum_utils.h"

template <class EnumType, class ValueType, typename Allocator = std::allocator<ValueType>>
struct IdIndexedMapping : std::vector<ValueType, Allocator>  //, true, std::underlying_type_t<EnumType>>
{
  using index_type = EnumType;
  using Container = std::vector<ValueType, Allocator>;  // , true, std::underlying_type_t<EnumType >> ;
  using Container::Container;

  ValueType& operator[](EnumType key)
  {
      return Container::operator[](nau::to_underlying(key));
  }

  const ValueType& operator[](EnumType key) const
  {
      return Container::operator[](nau::to_underlying(key));
  }

  // Hide parent's emplace_back: it's not useful and leads to errors
  auto emplace_back(...) = delete;

  // Appends a new mapping element and returns it's id and reference to it.
  // Should only be used when generating ids.
  template <class... Args>
  std::pair<EnumType, ValueType&> appendNew(Args&&... args)
  {
      NAU_FAST_ASSERT(Container::size() < std::numeric_limits<std::underlying_type_t<EnumType>>::max());
      auto newIndex = static_cast<EnumType>(Container::size());
      return {newIndex, Container::emplace_back(std::forward<Args>(args)...)};
  }

  // Automatically expands
  void set(EnumType key, ValueType value)
  {
    expandMapping(key);
    operator[](key) = value;
  }

  // Automatically expands
  ValueType &get(EnumType key, ValueType value)
  {
    expandMapping(key, value);
    return operator[](key);
  }

  ValueType &get(EnumType key)
  {
    expandMapping(key);
    return operator[](key);
  }

  bool isMapped(EnumType key) const
  {
      return nau::to_underlying(key) < Container::size();
  }

  void expandMapping(EnumType key, ValueType value)
  {
    if (!isMapped(key))
        Container::resize(nau::to_underlying(key) + 1, value);
  }

  // Has to be an overload to support move-only types
  void expandMapping(EnumType key)
  {
    if (!isMapped(key))
        Container::resize(nau::to_underlying(key) + 1);
  }

  IdEnumerateView<IdIndexedMapping> enumerate() & { return {*this}; }
  IdEnumerateView<const IdIndexedMapping> enumerate() const & { return {*this}; }
};
