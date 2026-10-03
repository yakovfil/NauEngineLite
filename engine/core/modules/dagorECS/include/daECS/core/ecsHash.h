// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <nau/utils/dag_hash.h>

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>

namespace ecs
{
typedef uint32_t hash_str_t;
}

template <int HashBits>
constexpr HashVal<HashBits> ecs_str_hash(const char *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return str_hash_fnv1<HashBits>(s, result);
}

template <int HashBits>
constexpr HashVal<HashBits> ecs_str_hash(const char8_t *s, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return str_hash_fnv1<HashBits>(s, result);
}

template <int HashBits>
constexpr HashVal<HashBits> ecs_mem_hash(const char *b, size_t len, HashVal<HashBits> result = FNV1Params<HashBits>::offset_basis)
{
  return mem_hash_fnv1<HashBits>(b, len, result);
}

constexpr ecs::hash_str_t ecs_mem_hash(const char *b, size_t len) { return ecs_mem_hash<32>(b, len); }
constexpr ecs::hash_str_t ecs_str_hash(const char *b) { return ecs_str_hash<32>(b); }
constexpr ecs::hash_str_t ecs_str_hash(const char8_t* b) {return ecs_str_hash<32>(b); }


namespace ecs
{
struct HashedConstString
{
  const char *str;
  hash_str_t hash;
  constexpr HashedConstString() = default;
  constexpr HashedConstString(const char8_t* str, hash_str_t hash) :
      str((const char*)(str)),
      hash(hash)
  {
  }
  constexpr HashedConstString(const char* str, hash_str_t hash) :
      str(str),
      hash(hash)
  {
  }
  constexpr HashedConstString(nullptr_t, hash_str_t hash) :
      str(nullptr),
      hash(hash)
  {
  }
};

#define ECS_HASH(a) (::ecs::HashedConstString({a, ::std::integral_constant<ecs::hash_str_t, ecs_str_hash(a)>::value}))
#define ECS_HASH_SLOW(a) (::ecs::HashedConstString({a, ecs_str_hash(a)}))

inline hash_str_t ecs_hash(std::string_view str)
{
    return ecs_mem_hash(str.data(), str.length());
}

struct EcsHasher
{
    size_t operator()(const std::string& str) const
    {
        return ecs_hash(str);
    }
};
struct EcsSvHasher
{
    size_t operator()(std::string_view str) const
    {
        return ecs_hash(str);
    }
};

} // namespace ecs
