// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved

#pragma once
#include "nau/threading/critical_section.h"
#include "nau/threading/dag_atomic.h"
#include "nau/diag/assertion.h"
namespace dag
{

#if _WIN32 | _TARGET_XBOX
    #include <windows.h>
    #define HAVE_WINAPI 1
    #define HAVE_LOCKCOUNT_IMPLEMENTATION 0
    #define PLATFORM_CRITICAL_SECTION_TYPE CRITICAL_SECTION
    static_assert(alignof(dag::CritSecStorage) == alignof(CRITICAL_SECTION));
#elif _TARGET_C1 | _TARGET_C2

#else
    #include <errno.h>
    #include <pthread.h>
    #include <time.h>
    #define HAVE_WINAPI 0
    #define PLATFORM_CRITICAL_SECTION_TYPE pthread_mutex_t
    #define HAVE_LOCKCOUNT_IMPLEMENTATION 1
#endif

    namespace csimpl
    {
        using type = PLATFORM_CRITICAL_SECTION_TYPE;
        inline type* get(void* p)
        {
            return (type*)p;
        }
        namespace lockcount
        {
#if HAVE_LOCKCOUNT_IMPLEMENTATION
            inline volatile int& ref(void* p)
            {
                return ((CritSecStorage*)p)->locksCount;
            }
            inline int value(void* p)
            {
                return ((CritSecStorage*)p)->locksCount;
            }
            static inline void release(void* p)
            {
                interlocked_release_store(lockcount::ref(p), 0);
            }
            static inline void increment(void* p)
            {
                interlocked_increment(lockcount::ref(p));
            }
            static inline void decrement(void* p)
            {
                NAU_ASSERT(lockcount::ref(p) > 0, "unlock not-locked CC? lockCount={}", lockcount::ref(p));
                interlocked_decrement(lockcount::ref(p));
            }
#else
            inline void release(void*)
            {
            }
            inline void increment(void*)
            {
            }
            inline void decrement(void*)
            {
            }
#endif
        }  // namespace lockcount
    }  // namespace csimpl

}  // namespace dag