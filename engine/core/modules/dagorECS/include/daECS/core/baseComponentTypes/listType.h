// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <daECS/core/component.h>
#include <daECS/utility/dag_vector.h>
#include <nau/math/math.h>

#include <utility>
#include <vector>

namespace ecs
{
class EntityComponentRef;
class EntityManager;
NAU_DAGORECS_EXPORT type_index_t find_component_type_index(component_type_t, ecs::EntityManager * = nullptr);

// Boolean components require addressable, contiguous bool values (not bit proxies).
template <typename T>
using ListStorage = std::conditional_t<std::is_same_v<T, bool>, dag::Vector<bool>, std::vector<T>>;

template <typename T>
class List : protected ListStorage<T>
{
public:
    typedef ListStorage<T> base_type;
    typedef typename base_type::iterator iterator;
    typedef typename base_type::reference reference;
    typedef typename base_type::const_iterator const_iterator;

    using typename base_type::value_type;
    // Warn: only RO methods are allowed to be reused from vector, RW methods have to call changeGen()
    using base_type::capacity;
    using base_type::cbegin;
    using base_type::cend;
    using base_type::empty;
    using base_type::reserve;
    using base_type::shrink_to_fit;

    uint32_t size() const
    {
        return uint32_t(base_type::size());
    }
  iterator begin()
  {
    changeGen();
    return base_type::begin();
  }
  iterator end() { return base_type::end(); }
  const_iterator begin() const { return base_type::begin(); }
  const_iterator end() const { return base_type::end(); }
  T *data()
  {
    changeGen();
    return base_type::data();
  }
  const T *data() const { return base_type::data(); } //-V659

  void clear()
  {
    changeGen();
    base_type::clear();
  }

  inline bool operator==(const List &a) const
  {
    if (size() != a.size())
      return false;
    for (size_t i = 0, e = size(); i < e; ++i)
    {
        if(!((base_type::operator[])(i) == ((const base_type&)a)[i]))
        {
            return false;
        }
    }
    return true;
  }

  T &front()
  {
    changeGen();
    return base_type::front();
  }
  const T &front() const { return base_type::front(); } //-V659

  T &back()
  {
    changeGen();
    return base_type::back();
  }
  const T &back() const { return base_type::back(); } //-V659

  T &push_back(T &&v)
  {
    changeGen();
    base_type::emplace_back(std::move(v));
    return base_type::back();
  }

  T &push_back(const T &v)
  {
    changeGen();
    base_type::emplace_back(v);
    return base_type::back();
  }

  T &push_back(ChildComponent &&v)
  {
    NAU_ASSERT(v.is<T>(), "Wrong type passed to ecs::List");

    changeGen();
    base_type::emplace_back(v.get<T>());
    return base_type::back();
  }

  iterator insert(const_iterator position, const T &v)
  {
    changeGen();
    return base_type::insert(position, v);
  }

  iterator insert(const_iterator position, T &&v)
  {
    changeGen();
    return base_type::insert(position, std::move(v));
  }

  iterator insert(const_iterator position, size_t n, const T &v)
  {
    changeGen();
    return base_type::insert(position, n, v);
  }

  iterator insert(const_iterator position, ChildComponent &&v)
  {
    NAU_ASSERT(v.is<T>(), "Wrong type passed to ecs::List");

    changeGen();
    return base_type::insert(position, std::move(v.get<T>()));
  }

  template <class InputIterator>
  iterator insert(const_iterator position, InputIterator first, InputIterator last)
  {
    changeGen();
    return base_type::insert(position, first, last);
  }

  template <class... Args>
  reference emplace_back(Args &&...args)
  {
    changeGen();
    return base_type::emplace_back(std::forward<Args>(args)...);
  }

  template <class... Args>
  iterator emplace(const_iterator position, Args &&...args)
  {
    changeGen();
    return base_type::emplace(position, std::forward<Args>(args)...);
  }

  void pop_back()
  {
    changeGen();
    base_type::pop_back();
  }

  iterator erase(const_iterator pos)
  {
    changeGen();
    return base_type::erase(pos);
  }
  iterator erase(const_iterator first, const_iterator last)
  {
    changeGen();
    return base_type::erase(first, last);
  }

  void resize(size_t sz)
  {
    changeGen();
    base_type::resize(sz);
  }
  void resize(size_t sz, const value_type &v)
  {
    changeGen();
    base_type::resize(sz, v);
  }

  T &operator[](size_t i)
  {
    changeGen();
    return (base_type::operator[])(i);
  }

  const T &operator[](size_t i) const //-V659
  {
    return (base_type::operator[])(i);
  }

  type_index_t getItemTypeId() const { return componentTypeIndex; }

  const auto getEntityComponentRef(ecs::EntityManager *mgr) const
  {
    type_index_t ti = find_component_type_index(ecs::ComponentTypeInfo<ecs::List<T>>::type, mgr);
    return EntityComponentRef((void *)(this), ecs::ComponentTypeInfo<ecs::List<T>>::type, ti, INVALID_COMPONENT_INDEX);
  }

  const auto getEntityComponentRef(size_t i) const
  {
    return EntityComponentRef((void *)(data() + i), ecs::ComponentTypeInfo<T>::type, getItemTypeId(), INVALID_COMPONENT_INDEX);
  }

  bool replicateCompare(const List &other)
  {
    if (gen == other.gen)
    {
#if NAU_DEBUG
      if (*this != other)
        {
          logerr("List differs, while it's generation is same");
        }
      else
        return false;
#else
      return false;
#endif
    }
    if (*this != other)
    {
      *this = other;
      gen = other.gen;
      return true;
    }
    gen = other.gen;
    return false;
  }

  List(EntityManager *emgr = nullptr)
  {
      if (NAU_UNLIKELY(componentTypeIndex == INVALID_COMPONENT_TYPE_INDEX))  // To consider: make INVALID_COMPONENT_TYPE_INDEX 0 for
                                                                             // faster comparisions like these
          componentTypeIndex = find_component_type_index(ComponentTypeInfo<T>::type, emgr);
  }

  List(const List &a) : base_type(static_cast<const base_type &>(a)) {}
  List(List &&a) : base_type(static_cast<base_type &&>(a)) {}
  inline List &operator=(const List &a)
  {
    ((base_type &)*this) = (const base_type &)a;
    changeGen();
    return *this;
  } // do not change generation
  inline List &operator=(List &&a)
  {
      ((base_type&)*this) = (base_type&&)std::move(a);
      changeGen();
      return *this;
  } // do not change generation

protected:
  static type_index_t componentTypeIndex;

  void changeGen() { gen++; }
  uint32_t gen = 0; // TODO: not all lists need to be replicatable, use different classes for non-replicatable lists (without
                    // replicateCompare and gen)
};

template <typename T>
type_index_t List<T>::componentTypeIndex = INVALID_COMPONENT_TYPE_INDEX;

}; // namespace ecs
