// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <vector>

#include "idEnumerateView.h"
#include "nau/utils/dynamic_bit_vector.h"
#include "nau/utils/enum_utils.h"

template <class EnumType, typename Allocator = std::allocator<nau::BitVectorWord>>
struct IdIndexedFlags : nau::DynamicBitVector<Allocator>
{
  using index_type = EnumType;
  using Container = nau::DynamicBitVector<Allocator>;
  using Container::Container;

  typename Container::reference operator[](EnumType key)
  {
      return Container::operator[](static_cast<typename Container::size_type>(nau::to_underlying(key)));
  }

  typename Container::const_reference operator[](EnumType key) const
  {
      return Container::operator[](static_cast<typename Container::size_type>(nau::to_underlying(key)));
  }

  void set(EnumType key, bool value)
  {
      Container::set(static_cast<typename Container::size_type>(nau::to_underlying(key)), value);
  }

  bool test(EnumType key, bool defaultValue = false) const
  {
      return Container::test(static_cast<typename Container::size_type>(nau::to_underlying(key)), defaultValue);
  }

  bool isMapped(EnumType key) const
  {
      return nau::to_underlying(key) < Container::size();
  }

  IdEnumerateView<IdIndexedFlags> enumerate() & { return {*this}; }
  IdEnumerateView<const IdIndexedFlags> enumerate() const & { return {*this}; }
};
