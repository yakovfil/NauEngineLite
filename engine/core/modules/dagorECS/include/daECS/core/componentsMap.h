// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "component.h"
#include "internal/archetypes.h"
#include "nau/utils/dynamic_bit_vector.h"
#include "nau/utils/inline_vector.h"
#include "nau/utils/sorted_vector.h"

namespace ecs
{
class ComponentsInitializer;
class ComponentsMap;

// typedef std::unordered_map<std::string, ChildComponent, EcsHasher,
//                         std::equal_to<std::string>,std::allocator<std::pair<const std::string, ChildComponent>>, /*bCacheHashCode*/true> BaseComponentsMap;
// typedef nau::SortedVectorMap<ecs::component_t, ChildComponent> BaseComponentsMap;
typedef nau::SortedVectorMap<component_t, ChildComponent, std::less<component_t>, std::allocator<std::pair<component_t, ChildComponent>>, std::vector<std::pair<component_t, ChildComponent>, std::allocator<std::pair<component_t, ChildComponent>>>>
    BaseComponentsMap;

// it is still extremely slow on (de)allocation. We'd better try some other container (Object from ecs20, for example)
class ComponentsMap : protected BaseComponentsMap
{
  friend class EntityManager;

public:
  typedef BaseComponentsMap base_type;
  using base_type::empty;
  size_t size() const { return base_type::size(); }
  base_type::iterator begin() { return base_type::begin(); }
  base_type::iterator end() { return base_type::end(); }
  base_type::const_iterator begin() const { return base_type::begin(); }
  base_type::const_iterator end() const { return base_type::end(); }
  void swap(ComponentsMap &other) { static_cast<BaseComponentsMap *>(this)->swap(*static_cast<BaseComponentsMap *>(&other)); }

  base_type::iterator find_as(const component_t c) { return base_type::find(c); }
  base_type::const_iterator find_as(const component_t c) const { return base_type::find(c); }
  base_type::iterator find_as(const HashedConstString s) { return find_as(s.hash); }
  base_type::const_iterator find_as(const HashedConstString s) const { return find_as(s.hash); }

  bool operator==(const ComponentsMap &a) const { return ((const base_type &)*this) == ((const base_type &)a); }

  template <class IteratorType>
  static hash_str_t getHash(const IteratorType &it)
  {
    return it->first;
  }

  template <class IteratorType>
  static component_index_t getComponentIndex(const DataComponents &dataComponents, const IteratorType &it)
  {
    return dataComponents.findComponentId(getHash(it));
  }

  ChildComponent &operator[](const ecs::component_t name_hash) { return (base_type::operator[](name_hash)); }

  ChildComponent &operator[](const HashedConstString name) { return (base_type::operator[](name.hash)); }

  const ChildComponent *find(const component_t name) const;
  const ChildComponent *find(const HashedConstString name) const { return find(name.hash); }
  const ChildComponent &operator[](const HashedConstString name) const;
  // comment it to avoid careless use of slow operation
  base_type::iterator find_as(const char *s) { return base_type::find(ECS_HASH_SLOW(s).hash); }
  ChildComponent &operator[](const char *name) { return this->operator[](ECS_HASH_SLOW(name)); }
  const ChildComponent &operator[](const char *name) const { return this->operator[](ECS_HASH_SLOW(name)); }
  using base_type::shrink_to_fit;
};

struct InitializerNode
{
  ChildComponent second;
  component_t name;
  component_index_t cIndex; // initialized once during validation
};

typedef nau::InlineVector<InitializerNode, 8, true> BaseComponentsInitializer;

class ComponentsInitializer : public BaseComponentsInitializer //TODO NEVER INHERIT FROM CLASSES FROM STD OR EASTL!!!!!!!!!!!!! 
{
public:
  typedef BaseComponentsInitializer base_type;
  using base_type::empty;
  size_t size() const { return base_type::size(); }
  base_type::iterator begin() { return base_type::begin(); }
  base_type::iterator end() { return base_type::end(); }
  base_type::const_iterator begin() const { return base_type::begin(); }
  base_type::const_iterator end() const { return base_type::end(); }

  ChildComponent &operator[](const HashedConstString name); // always create new value
  // ChildComponent& operator[](const char* name) { return this->operator[](ECS_HASH_SLOW(name)); }
  template <class IteratorType>
  static hash_str_t getHash(const IteratorType &it)
  {
    return it->name;
  }

  template <class IteratorType>
  static component_index_t getComponentIndex(const DataComponents &dataComponents, const IteratorType &it)
  {
    return dataComponents.findComponentId(it->name);
  }

  template <class ComponentType>
  bool insert(const component_t name, const FastGetInfo &lt, ComponentType &&val, const char *nameStr);
  void reserve(size_t sz) { base_type::reserve(sz); }

  // explicit
  // todo: should be explicit, but we keep it implicit during transition
  explicit ComponentsInitializer(ComponentsMap &&map);
  // Rule-of-5
  ComponentsInitializer() = default;
  ComponentsInitializer(size_t sz) { reserve(sz); }
  ComponentsInitializer(const ComponentsInitializer &) = default;
  ComponentsInitializer(ComponentsInitializer &&) = default;
  ComponentsInitializer &operator=(const ComponentsInitializer &) = default;
  ComponentsInitializer &operator=(ComponentsInitializer &&) = default;
  ~ComponentsInitializer() = default;
  friend class EntityManager;
};

inline const ChildComponent *ComponentsMap::find(const component_t name) const
{
  auto it = find_as(name);
  if (it == base_type::end())
    return NULL;
  return &it->second;
}

inline const ChildComponent &ComponentsMap::operator[](const HashedConstString name) const
{
  auto it = find_as(name);
  if (it == base_type::end())
    return ChildComponent::get_null();
  return it->second;
}

template <class ComponentType>
__forceinline bool ComponentsInitializer::insert(const component_t name, const FastGetInfo &lt, ComponentType &&val,
  const char *nameStr)
{
    if (NAU_UNLIKELY(!lt.isValid()))
        return false;
    NAU_UNUSED(nameStr);
    base_type::emplace_back(InitializerNode{ChildComponent(std::forward<ComponentType>(val)), name, lt.getCidx()});
    return true;
}

__forceinline ChildComponent &ComponentsInitializer::operator[](const HashedConstString name)
{
  auto val = name.hash;
  base_type::emplace_back(InitializerNode{ChildComponent(), val, INVALID_COMPONENT_INDEX});
  return base_type::back().second;
}

__forceinline ComponentsInitializer::ComponentsInitializer(ComponentsMap &&map)
{
  for (auto it = map.begin(); it != map.end(); ++it)
  {
    auto val = it->first;
    base_type::emplace_back(InitializerNode{std::move(it->second), val, INVALID_COMPONENT_INDEX});
  }
};
}; // namespace ecs
