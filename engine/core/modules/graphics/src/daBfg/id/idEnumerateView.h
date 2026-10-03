// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include "nau/utils/dynamic_bit_vector.h"

template <class Iter, class = void>
struct ReferenceTypePicker
{
    using Type = typename std::iterator_traits<Iter>::reference;
};

// Workaround for a "bug" inside nau::DynamicBitVector, whose reference type
// is declared incorrectly
template <class T>
struct ReferenceTypePicker<T, std::void_t<typename T::reference_type>>
{
  using Type = typename T::reference_type;
};

// Inspired by P2164
template <class Container>
struct IdEnumerateView
{
    using Base = std::remove_cv_t<Container>;
    using BaseIterator = decltype(std::declval<Container>().begin());

    using size_type = typename Base::size_type;
    using index_type = typename Base::index_type;
    using value_type = std::tuple<const index_type, typename ReferenceTypePicker<BaseIterator>::Type>;

    struct iterator;

    iterator begin()
    {
        return {0, container.begin()};
    }
  iterator end() { return {container.size(), container.end()}; }

  iterator begin() const { return {0, container.begin()}; }
  iterator end() const { return {container.size(), container.end()}; }

  Container &container;
};

template <class Container>
struct IdEnumerateView<Container>::iterator
{

  // No need to provide anything stronger than a forward iterator, as
  // we only support a single usecase:
  // `for (auto[i, obj] : idIndexedThing.enumerate())`
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = typename std::iterator_traits<BaseIterator>::difference_type;
  using value_type = typename IdEnumerateView<Container>::value_type;
  using reference = value_type;
  using pointer = void;


  auto operator*() { return reference{static_cast<index_type>(pos), *current}; }
  iterator &operator++()
  {
    ++current;
    ++pos;
    return *this;
  }
  iterator &operator--()
  {
    --current;
    --pos;
    return *this;
  }
  iterator operator++(int)
  {
    iterator tmp = *this;
    ++*this;
    return tmp;
  }
  iterator operator--(int)
  {
    iterator tmp = *this;
    --*this;
    return tmp;
  }

  friend bool operator==(const iterator &fst, const iterator &snd) { return fst.current == snd.current; }
  friend bool operator!=(const iterator &fst, const iterator &snd) { return !(fst == snd); }

  decltype(std::declval<Container>().size()) pos;
  BaseIterator current;
};
