// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

#include <windows.h>

#include <memory>

struct UnloadLibHandler
{
  typedef HMODULE pointer;
  void operator()(HMODULE lib)
  {
    if (lib)
    {
      FreeLibrary(lib);
    }
  }
};

using LibPointer = std::unique_ptr<HMODULE, UnloadLibHandler>;

struct GenericHandleHandler
{
  typedef HANDLE pointer;
  void operator()(pointer h)
  {
    if (h != nullptr && h != INVALID_HANDLE_VALUE)
      CloseHandle(h);
  }
};

using HandlePointer = std::unique_ptr<HANDLE, GenericHandleHandler>;
using EventPointer = std::unique_ptr<HANDLE, GenericHandleHandler>;

struct VirtaulAllocMemoryHandler
{
  void operator()(void *ptr) { VirtualFree(ptr, 0, MEM_RELEASE); }
};

template <typename T>
using VirtualAllocPtr = std::unique_ptr<T, VirtaulAllocMemoryHandler>;
