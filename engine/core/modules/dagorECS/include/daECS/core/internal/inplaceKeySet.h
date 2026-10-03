// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <nau/utils/dag_relocatableFixedVector.h>
#include <nau/utils/dag_fixedVectorSet.h>

template <int bytes>
struct SizeTypeBytes
{
  typedef uint32_t size_type_t;
};
template <>
struct SizeTypeBytes<2>
{
  typedef uint16_t size_type_t;
};
template <>
struct SizeTypeBytes<1>
{
  typedef uint8_t size_type_t;
};

template <class RelocatableKey, size_t inplace_count = 16 / sizeof(RelocatableKey), class SizeType = typename SizeTypeBytes<sizeof(RelocatableKey)>::size_type_t>
using InplaceKeyContainer = dag::RelocatableFixedVector<RelocatableKey, inplace_count, true, nau::HeapByteAllocator /*, MidmemAlloc TODO Allocators.*/, SizeType>;

template <class RelocatableKey, size_t inplace_count = 16 / sizeof(RelocatableKey), class SizeType = typename SizeTypeBytes<sizeof(RelocatableKey)>::size_type_t>
using InplaceKeySet = dag::FixedVectorSet<RelocatableKey, inplace_count, true, std::identity, nau::HeapByteAllocator /*, MidmemAlloc TODO Allocators.*/, SizeType>;
