// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "nau/threading/event.h"

#include "nau/diag/assertion.h"
#include "nau/threading/lock_guard.h"

#if !(NAU_THREADING_EVENT_STDLIB)
    #include <synchapi.h>
#endif

namespace nau::threading
{

#if NAU_THREADING_EVENT_STDLIB

    Event::Event(ResetMode mode, bool signaled) :
        m_mode(mode),
        m_state(signaled)
    {
    }

    Event::~Event() = default;

    void Event::reset()
    {
        lock_(m_mutex);
        m_state = false;
    }

    void Event::set()
    {
        lock_(m_mutex);

        m_state = true;

        if(m_mode == ResetMode::Auto)
        {
            m_signal.notify_one();
        }
        else
        {
            m_signal.notify_all();
        }
    }

    bool Event::wait(std::optional<std::chrono::milliseconds> timeout)
    {
        std::unique_lock lock{m_mutex};

        bool ready = false;
        if(timeout)
        {
            ready = m_signal.wait_for(lock, *timeout, [this]
                                      {
                                          return m_state;
                                      });
        }
        else
        {
            m_signal.wait(lock, [this]
                          {
                              return m_state;
                          });
            NAU_ASSERT(m_state);
            ready = true;
        }

        if(ready && m_mode == ResetMode::Auto)
        {
            m_state = false;
        }

        return ready;
    }

#else  // NAU_THREADING_EVENT_STDLIB

    static_assert(sizeof(void*) == sizeof(HANDLE) && std::is_same_v<void*, HANDLE>);

    Event::Event(ResetMode mode, bool signaled) :
        m_mode(mode),
        m_hEvent(::CreateEventW(nullptr, (mode == ResetMode::Manual) ? TRUE : FALSE, signaled ? TRUE : FALSE, nullptr))
    {
        NAU_ASSERT(m_hEvent);
    }

    Event::~Event()
    {
        if(m_hEvent)
        {
            ::CloseHandle(m_hEvent);
        }
    }

    void Event::set()
    {
        ::SetEvent(m_hEvent);
    }

    void Event::reset()
    {
        ::ResetEvent(m_hEvent);
    }

    bool Event::wait(std::optional<std::chrono::milliseconds> timeout)
    {
        const DWORD ms = timeout ? static_cast<DWORD>(timeout->count()) : INFINITE;

        const auto waitResult = WaitForSingleObject(m_hEvent, ms);

        if(waitResult == WAIT_OBJECT_0)
        {
            return true;
        }

        if(waitResult != WAIT_TIMEOUT)
        {
        }

        return false;
    }

#endif  // NAU_THREADING_EVENT_STDLIB

    Event::ResetMode Event::getMode() const
    {
        return m_mode;
    }

}  // namespace nau::threading

#ifdef NO_THREADS

namespace nau::threading
{
    Event::Event(ResetMode mode, [[maybe_unused]] bool signaled) :
        m_mode(mode)
    {
    }

    Event::~Event() = default;

    void Event::set()
    {
    }

    bool Event::wait(std::optional<std::chrono::milliseconds>)
    {
        NAU_FAILURE_ALWAYS("Event::Wait can not be used with no thread support");
        return true;
    }

}  // namespace nau::threading

#endif  // NO_THREADS
