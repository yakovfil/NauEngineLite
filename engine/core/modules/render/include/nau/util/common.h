// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <array>
#include <span>
#include <type_traits>
#include <vector>

#include "nau/memory/heap_byte_allocator.h"

extern nau::HeapByteAllocator* midmem;
extern nau::HeapByteAllocator* inimem;
extern nau::HeapByteAllocator* strmem;
extern nau::HeapByteAllocator* tmpmem;
extern nau::HeapByteAllocator* globmem;

#if _TARGET_PC_WIN | _TARGET_XBOX
wchar_t *utf8_to_wcs(const char *utf8_str, wchar_t *wcs_buf, int wcs_buf_len);
#endif


template <class V, typename U = typename V::value_type>
inline void mem_copy_to(const V &v, void *mem_dest)
{
  memcpy(mem_dest, v.data(), data_size(v));
}

inline void memfree(void* p, nau::HeapByteAllocator* aloc)
{
    aloc->deallocate(p, 0);
}

inline void* memalloc(size_t sz, nau::HeapByteAllocator* aloc)
{
    return aloc->allocate(sz);
}


template <class V, typename T = typename V::value_type>
inline void erase_items(V &v, uint32_t at, uint32_t n)
{
  if (n == 1) // This branch typically should be optimized away
    v.erase(v.begin() + at);
  else
    v.erase(v.begin() + at, v.begin() + at + n);
}


#define DAGOR_LIKELY(x)   (x)
#define DAGOR_UNLIKELY(x) (x)


template <class V, typename T = typename V::value_type>
inline void clear_and_resize(V &v, uint32_t sz)
{
  if (sz == v.size())
    return;
  v.clear();
  v.resize(sz);
}


template <typename V, typename T = typename V::value_type>
inline uint32_t data_size(const V &v)
{
  return v.size() * (uint32_t)sizeof(T);
}

template <class T, size_t S>
inline void mem_set_0(std::span<T, S>& v)
{
  memset(v.data(), 0, data_size(v));
}

template <class T, size_t S>
inline void mem_set_0(const std::span<T, S>& v)
{
  memset(v.data(), 0, data_size(v));
}

template <class T, size_t S>
inline void mem_set_0(std::array<T, S>& v)
{
  memset(v.data(), 0, S);
}

template <class T>
inline void mem_set_0(std::vector<T>& v)
{
  memset(v.data(), 0, data_size(v));
}

inline char* str_dup(const char* s, nau::HeapByteAllocator* a)
{
  // dbg( "%s : %i  (%s)\n", __FILE__, __LINE__, __FUNCTION__ );
  if (!s)
    return NULL;
  size_t n = strlen(s) + 1;
  void *p = a->allocate(n);
  if (!p)
    return (char *)p;
  memcpy(p, s, n);
  return (char *)p;
}

inline const char* get_log_directory()
{
    static const char* log_dir = "./";
    return log_dir;
}


template <typename To, typename From>
__forceinline To bitwise_cast(const From &from)
{
  static_assert(sizeof(To) == sizeof(From), "bitwise_cast: types of different sizes provided");
  static_assert(std::is_pointer<To>::value == std::is_pointer<From>::value,
                "bitwise_cast: can't cast between pointer and non-pointer types");
  static_assert(sizeof(From) <= 16, "bitwise_cast: can't cast types of size > 16");
  To result;
  memcpy(&result, &from, sizeof(From));
  return result;
}


#if defined(__cplusplus) && !defined(__GNUC__)
template <typename T, size_t N>
char (&_countof__helper_(T (&array)[N]))[N];
#define countof(x) (sizeof(_countof__helper_(x)))
#else
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif
#define c_countof(x) (sizeof(x) / sizeof((x)[0]))


#define G_VERIFY(expression) \
  do                         \
  {                          \
    (void)(expression);      \
  } while (0)
#define G_VERIFYF(expression, fmt, ...) \
  do                                    \
  {                                     \
    (void)(expression);                 \
  } while (0)
