// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once

// #include <debug/daNAU_ASSERT.h>
#include <string>
#if (_TARGET_PC_WIN /*|| _TARGET_XBOX*/) && DAGOR_DBGLEVEL > 0
#include <intrin.h>
#define COMMANDS_STORE_RETURN_ADDRESS 1
#endif
#include "nau/string/string.h"

#if _TARGET_C1



#endif

__forceinline const void *get_return_address()
{
#if _TARGET_PC_WIN || _TARGET_XBOX
  return _ReturnAddress();
#elif _TARGET_C1

#else
  return nullptr;
#endif
}

struct ScopedReturnAddressStore
{
  ScopedReturnAddressStore(const void *return_address);
  ~ScopedReturnAddressStore();
  static const void *get_threadlocal_saved_address();

private:
  const void *localAddress;
};

#if COMMANDS_STORE_RETURN_ADDRESS
#define STORE_RETURN_ADDRESS() ScopedReturnAddressStore scopedReturnAddressStore(get_return_address())
#else
#define STORE_RETURN_ADDRESS() (void)0
#endif

namespace lockreturnaddr
{
void before_lock(const void *return_address);
void after_successful_lock(const void *return_address);
void after_failed_lock(const void *return_address);
void before_unlock(int lock_count);
nau::string ptrs_to_string();
bool validate_addr_store(bool is_locked);
} // namespace lockreturnaddr

#if LOCKS_STORE_RETURN_ADDRESS
#define BEFORE_LOCK()             lockreturnaddr::before_lock(get_return_address())
#define AFTER_SUCCESSFUL_LOCK()   lockreturnaddr::after_successful_lock(get_return_address())
#define AFTER_FAILED_LOCK()       lockreturnaddr::after_failed_lock(get_return_address())
#define BEFORE_UNLOCK(lock_count) lockreturnaddr::before_unlock(lock_count)
#if LOCKS_RETURN_ADDRESS_VALIDATION_ENABLED
#define VALIDATE_LOCK_ADDRESSES(is_locked) NAU_ASSERT(lockreturnaddr::validate_addr_store(is_locked))
#else
#define VALIDATE_LOCK_ADDRESSES(is_locked) (void)0
#endif
#else
#define BEFORE_LOCK()                      (void)0
#define AFTER_SUCCESSFUL_LOCK()            (void)0
#define AFTER_FAILED_LOCK()                (void)0
#define BEFORE_UNLOCK(count)               (void)0
#define VALIDATE_LOCK_ADDRESSES(is_locked) (void)0
#endif
