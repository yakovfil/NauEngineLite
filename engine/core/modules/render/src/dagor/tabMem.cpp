// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "nau/memory/heap_byte_allocator.h"
#include "nau/util/dag_tabHlp.h"

void* dag_tab_insert2(void* ptr, uint32_t& total, uint32_t& used, nau::HeapByteAllocator* mem, intptr_t at, intptr_t n, intptr_t sz, intptr_t step, uint32_t& out_idx)
{
  if (n <= 0)
  {
    out_idx = at;
    return ptr;
  }
  if (at > used)
    at = used;
  int nn = used + n;
  if (nn > total)
  {
    if (step)
      nn = ((nn + step - 1) / step) * step;
    else
    {
      int newtotal = total ? (total + (total + 1) / 2) : (16 / sz);
      if (newtotal > nn)
        nn = newtotal;
    }

    const size_t atSize = at * sz;
    const size_t addSize = n * sz;
    const size_t newSize = nn * sz;
    if (at == used)
    {
      total = nn;
      ptr = mem->allocate(newSize);//mem->realloc(ptr, newSize);
    }
    else // it is not append. sometimes it still make sense to reallocate, but heuristics are tricky, so just fallback to alloc/free
    {
      size_t asz = newSize;
      void *nt = mem->allocate(newSize);
      if (at)
        memcpy(nt, ptr, atSize);
      if (used - at)
        memcpy((char *)nt + atSize + addSize, (char *)ptr + atSize, (used - at) * sz);
      if (ptr)
        mem->deallocate(ptr, 0);
      ptr = nt;
      total = asz / sz;
    }
    used += n;
  }
  else
  {
    if (used - at)
      memmove((char *)ptr + (at + n) * sz, (char *)ptr + at * sz, (used - at) * sz);
    used += n;
  }
  out_idx = at;
  return ptr;
}

