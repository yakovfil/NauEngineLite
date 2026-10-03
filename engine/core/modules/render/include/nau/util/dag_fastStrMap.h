// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <functional>

#include "nau/memory/heap_byte_allocator.h"
// #include <memory/dag_mem.h>
// #include "nau/generic/dag_tabSort.h"
#include <stdint.h>
#include <string.h>

#include <span>
#include <unordered_map>

#include "nau/osApiWrappers/dag_localConv.h"
#include "nau/util/common.h"
#include "nau/utils/span.h"

/// Fast string map (add string/strId, get strId by string)
template <typename T, intptr_t invalidId = -1>
class NauFastStrMapT
{
public:
  typedef T IdType;
  struct Entry
  {
    const char *name;
    IdType id;
  };


public:
    NauFastStrMapT(nau::HeapByteAllocator* mem = NULL, bool ignore_case = false) :
        stringAllocator(mem ? mem : tmpmem),
        ignoreCase(ignore_case)
    {
    }

  //NauFastStrMapT(const FastStrMapT &m) : fastMap(m.fastMap), ignoreCase(false)
  //{
  //  //for (int i = 0; i < fastMap.size(); i++)
  //  //  fastMap[i].name = str_dup(fastMap[i].name, dag::get_allocator(fastMap));
  //}

  ~NauFastStrMapT() { reset(false); }

  NauFastStrMapT(const NauFastStrMapT &m) = delete;
  NauFastStrMapT &operator=(const NauFastStrMapT &m) = delete;

  //NauFastStrMapT &operator=(const NauFastStrMapT &m)
  //{
  //  reset();
  //  fastMap = m.fastMap;
  //  for (int i = 0; i < fastMap.size(); i++)
  //    fastMap[i].name = str_dup(fastMap[i].name, dag::get_allocator(fastMap));
  //  return *this;
  //}


  size_t strToHash(const char* name) const
  {
      nau::string naustr;
      if(ignoreCase)
      {
        naustr = name; // TODO: make str into lower case
      }
      else
      {
        naustr = name;
      }
      std::hash<nau::string> hasher;
      size_t hash = hasher(naustr);

      return hash;
  }


  /// Returns strId for given name or invalidId if name not found.
  IdType getStrId(const char *name) const
  {
      size_t hash = strToHash(name);

      if(fastMap.count(hash))
      {
          return fastMap.at(hash).id;
      }

      return (IdType)invalidId;
  }

  /// Returns strId for given name (adds str to the list if not found)
  IdType addStrId(const char *str, IdType id, const char *&stored_str)
  {
      size_t hash = strToHash(str);

      if(fastMap.count(hash))
      {
          stored_str = fastMap[hash].name;
          return fastMap[hash].id;
      }

      Entry val = {
          str_dup(str, stringAllocator),
          id};

      stored_str = val.name;
      fastMap[hash] = val;

      return id;

    //int str_index = getStrIndex(str);
    //if (str_index < 0)
    //{
    //  Entry key = {str_dup(str, dag::get_allocator(fastMap)), id};
    //  if (ignoreCase)
    //    fastMap.insert(key, LambdaStricmp());
    //  else
    //    fastMap.insert(key, LambdaStrcmp());
    //
    //  stored_str = key.name;
    //  return id;
    //}
    //
    //stored_str = fastMap[str_index].name;
    //return fastMap[str_index].id;
  }

  /// Returns strId for given name (adds str to the list if not found)
  IdType addStrId(const char *str, IdType id)
  {
      size_t hash = strToHash(str);

      if(fastMap.count(hash))
      {
          return fastMap[hash].id;
      }

      Entry val = {
          str_dup(str, stringAllocator),
          id};

      fastMap[hash] = val;

      return id;


    //IdType str_id = getStrId(str);
    //if (str_id == (IdType)invalidId)
    //{
    //  str_id = id;
    //  Entry key = {str_dup(str, dag::get_allocator(fastMap)), str_id};
    //  if (ignoreCase)
    //    fastMap.insert(key, LambdaStricmp());
    //  else
    //    fastMap.insert(key, LambdaStrcmp());
    //}
    //
    //return str_id;
  }

  /// Set strId for given name (adds str to the list if not found and returns true)
  bool setStrId(const char *str, IdType id)
  {
      size_t hash = strToHash(str);

      if(fastMap.count(hash))
      {
          fastMap[hash].id = id;
          return false;
      }

      Entry val = {
          str_dup(str, stringAllocator),
          id};

      fastMap[hash] = val;

      return true;

    //int idx = getStrIndex(str);
    //
    //if (idx == -1)
    //{
    //  Entry key = {str_dup(str, dag::get_allocator(fastMap)), id};
    //  if (ignoreCase)
    //    fastMap.insert(key, LambdaStricmp());
    //  else
    //    fastMap.insert(key, LambdaStrcmp());
    //  return true;
    //}
    //else
    //{
    //  fastMap[idx].id = id;
    //  return false;
    //}
  }

  bool delStrId(const char *str)
  {
    size_t hash = strToHash(str);

    if(fastMap.count(hash))
    {
        return false;
    }

    memfree((void*)fastMap[hash].name, stringAllocator);

    fastMap.erase(hash);
    return true;

    //int idx = getStrIndex(str);
    //if (idx == -1)
    //  return false;
    //memfree((void *)fastMap[idx].name, dag::get_allocator(fastMap));
    //erase_items(fastMap, idx, 1);
    //return true;
  }

  bool delStrId(IdType str_id)
  {
      return false;
    //int cnt = fastMap.size();
    //for (int i = fastMap.size() - 1; i >= 0; i--)
    //  if (fastMap[i].id == str_id)
    //  {
    //    memfree((void *)fastMap[i].name, dag::get_allocator(fastMap));
    //    erase_items(fastMap, i, 1);
    //  }
    //return fastMap.size() < cnt;
  }

  /// returns number of strings
  int strCount() const { return fastMap.size(); }

  /// Resets strmap to initial state (all previously issued ids become invalid)
  void reset(bool erase_only = true)
  {
    //for (int i = fastMap.size() - 1; i >= 0; i--)
    //  memfree((void *)fastMap[i].name, dag::get_allocator(fastMap));

    erase_only ? fastMap.clear() : nau::clear_and_shrink(fastMap);
  }

  /// Reserves memory for at least add_num additinal strs
  void reserve(int add_num)
  {
      fastMap.reserve(fastMap.size() + add_num);
  }

  /// Returns non-modifiable slice of entries
  std::span<const Entry> getMapRaw() const
  {
      return fastMap;
  }

  /// Returns index in map array for given str or -1 if str not found.
  int getStrIndex(const char *str) const
  {
      size_t hash = strToHash(str);

      if (fastMap.count(hash))
      {
          return fastMap.at(hash).id;
      }

    return invalidId;
  }

  /// Walks through all map to find str_id (slow); returns NULL when str_id is not found
  const char *getStrSlow(IdType str_id) const
  {
    //for (const Entry &e : fastMap)
    //  if (e.id == str_id)
    //    return e.name;
    return NULL;
  }

protected:
  //TabSortedFast<Entry> fastMap;
    nau::HeapByteAllocator* stringAllocator;
    std::unordered_map<size_t, Entry> fastMap;
    bool ignoreCase;
};

#ifdef _TARGET_STATIC_LIB
extern template class FastStrMapT<int, -1>;
extern template class FastStrMapT<int, 0>;
extern template class FastStrMapT<char *, 0>;
#endif

typedef NauFastStrMapT<int, -1> FastStrMap;
