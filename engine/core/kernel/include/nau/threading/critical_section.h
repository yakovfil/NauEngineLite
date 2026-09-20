// Copyright 2024 N-GINN LLC. All rights reserved.
// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <cstddef>
#include "nau/kernel/kernel_config.h"

namespace dag
{

#if _TARGET_APPLE | _TARGET_ANDROID | _TARGET_64BIT
    static constexpr int CRITICAL_SECTION_OBJECT_SIZE = 64;
#else
    static constexpr int CRITICAL_SECTION_OBJECT_SIZE = 40;
#endif

    struct CritSecStorage
    {
#if defined(__GNUC__)
        char critSec[CRITICAL_SECTION_OBJECT_SIZE] __attribute__((aligned(16)));
        volatile int locksCount;
#elif defined(_MSC_VER)
        alignas(void*) char critSec[CRITICAL_SECTION_OBJECT_SIZE];
#else
    #error Compiler is not supported
#endif
        operator void*()
        {
            return critSec;
        }
    };

    //
    // critical sections
    //
    NAU_KERNEL_EXPORT void create_critical_section(void* cc_storage, const char* name = NULL);
    NAU_KERNEL_EXPORT void destroy_critical_section(void* cc);
    NAU_KERNEL_EXPORT void enter_critical_section_raw(void* cc);                                   // no profiling
    NAU_KERNEL_EXPORT void enter_critical_section(void* cc, const char* waiter_perf_name = NULL);  // with profiling
    NAU_KERNEL_EXPORT void leave_critical_section(void* cc);
    NAU_KERNEL_EXPORT bool try_enter_critical_section(void* cc);

    NAU_KERNEL_EXPORT int full_leave_critical_section(void* cc);
    NAU_KERNEL_EXPORT void multi_enter_critical_section(void* cc, int enter_cnt);
    NAU_KERNEL_EXPORT bool try_timed_enter_critical_section(void* cc, int timeout_ms, const char* waiter_perf_name = NULL);

    //
    // Critical section wrapper
    //
    class CriticalSection
    {
    private:
        // make copy constructor and assignment operator inaccessible
        CriticalSection(const CriticalSection& refCritSec);
        CriticalSection& operator=(const CriticalSection& refCritSec);

        friend class CSAutoLock;

    protected:
        CritSecStorage critSec;

    public:
        CriticalSection(const char* name = NULL)
        {
            create_critical_section(critSec, name);
        }  //-V730
        ~CriticalSection()
        {
            destroy_critical_section(critSec);
        }
        void lock(const char* wpn = NULL)
        {
            enter_critical_section(critSec, wpn);
        }
        bool tryLock()
        {
            return try_enter_critical_section(critSec);
        }
        bool timedLock(int timeout_ms, const char* wpn = NULL)
        {
            return try_timed_enter_critical_section(critSec, timeout_ms, wpn);
        }
        void unlock()
        {
            leave_critical_section(critSec);
        }
        int fullUnlock()
        {
            return full_leave_critical_section(critSec);
        }
        void reLock(int cnt)
        {
            multi_enter_critical_section(critSec, cnt);
        }
    };


    //
    // Auto-lock object;
    //   locks a critical section, and unlocks it automatically when the lock goes out of scope
    //
    class CSAutoLock
    {
    private:
      // make copy constructor and assignment operator inaccessible
      CSAutoLock(const CSAutoLock &refAutoLock) = delete;
      CSAutoLock &operator=(const CSAutoLock &refAutoLock) = delete;
    
    protected:
      CritSecStorage *pLock;
    
      // Unavailable, use WinAutoLockOpt if you need these
      CSAutoLock(CritSecStorage *pcss) : pLock(pcss) { lock(); }
      CSAutoLock(CriticalSection *pwcs) : CSAutoLock(&pwcs->critSec) {}
    
    public:
      CSAutoLock(CritSecStorage &css) : CSAutoLock(&css) {}
      CSAutoLock(CriticalSection &wcs) : CSAutoLock(&wcs) {}
      ~CSAutoLock() { unlock(); }
      void lock(const char *wpn = NULL)
      {
        if (pLock)
          enter_critical_section(pLock, wpn);
      }
      void unlock()
      {
        if (pLock)
          leave_critical_section(pLock);
      }
      void unlockFinal()
      {
        unlock();
        pLock = NULL;
      }
    };

    class CSAutoLockOpt : public CSAutoLock
    {
    public:
        CSAutoLockOpt(CriticalSection *pwcs) : CSAutoLock(pwcs) {}
        CSAutoLockOpt(CritSecStorage *pcss) : CSAutoLock(pcss) {}
    };


}  // namespace nau
