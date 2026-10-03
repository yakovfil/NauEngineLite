// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <memory>
#include <utility>
#include <vector>
#ifndef RESTRICT_FUN
#ifdef _MSC_VER
#define RESTRICT_FUN __declspec(restrict)
#else
#define RESTRICT_FUN
#endif
#endif

// this is simple allocator, which works almost as region buffer (chain of chunks of equal size).
// however, if you work with it as stack (alloc0,alloc1,..allocN | deallocN,..alloc1,alloc0)
// it will perform like a stack (not allocating new data)
// it never deallocates memory, until Garbage collection called.
// it will always have at least one chunk of allocated data
// it always return pointers to aligned data (although we assume that new is aligned to same alignment. This assumption be very easily
// removed, see assert)

template <unsigned CHUNK_SIZE_BITS = 16, unsigned ALIGN_TO_BITS = 4>
struct StackAllocator
{
  enum
  {
    ALIGN_TO = 1 << ALIGN_TO_BITS,
    ALIGN_TO_MASK = ALIGN_TO - 1
  }; // pow-of-two size
  enum
  {
    CHUNK_SIZE = 1 << CHUNK_SIZE_BITS,
    CHUNK_MASK = CHUNK_SIZE - 1
  }; // pow-of-two size
  struct Chunk
  {
      std::unique_ptr<uint8_t[]> data = std::make_unique<uint8_t[]>(CHUNK_SIZE);
      size_t allocated = 0;
  };
  Chunk activeChunk;
  std::vector<Chunk> chunks;

  NAU_NOINLINE
  void pushChunk()
  {
      std::swap(activeChunk, chunks.emplace_back());
      if (ALIGN_TO > 16)
          activeChunk.allocated += (ALIGN_TO - (uintptr_t(activeChunk.data.get()) & ALIGN_TO_MASK)) & ALIGN_TO_MASK;
  }
  NAU_NOINLINE
  void popChunk()
  {
      std::swap(activeChunk, chunks.back());
      chunks.pop_back();
  }
  StackAllocator() {}

  RESTRICT_FUN uint8_t *__restrict allignedAllocate(uint32_t size)
  {
    NAU_FAST_ASSERT((size & ALIGN_TO_MASK) == 0);
    if (NAU_UNLIKELY(activeChunk.allocated + size > CHUNK_SIZE))
        pushChunk();
    uint8_t *data = activeChunk.data.get() + activeChunk.allocated;
    activeChunk.allocated += size;
    return data;
  }
  RESTRICT_FUN uint8_t *__restrict allocate(uint32_t &size)
  {
    size = (size + ALIGN_TO_MASK) & ~ALIGN_TO_MASK; // align to 16
    return allignedAllocate(size);
  }

  void deallocate(uint8_t *__restrict data, uint32_t size)
  {
    NAU_UNUSED(data);
    NAU_ASSERT(data >= activeChunk.data.get() && data - activeChunk.data.get() + size == activeChunk.allocated);
    activeChunk.allocated -= size;
    if (NAU_UNLIKELY(activeChunk.allocated <= ALIGN_TO_MASK && !chunks.empty()))
        popChunk();
  }

  size_t calcMemAllocated() const // for debug/inspection
  {
    size_t sz = (activeChunk.allocated & (~ALIGN_TO_MASK));
    for (const auto &ch : chunks)
      sz += (ch.allocated & (~ALIGN_TO_MASK));
    return sz;
  }
  void clear()
  {
    chunks.clear();
    pushChunk();
  }
};
