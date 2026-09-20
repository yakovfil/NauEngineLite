// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// nau/threading/event.h


#pragma once
#include <nau/core_defines.h>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "nau/kernel/kernel_config.h"

#ifdef NAU_PLATFORM_WIN32
    #define NAU_THREADING_EVENT_STDLIB 0
#else
    #define NAU_THREADING_EVENT_STDLIB 1
#endif

namespace nau::threading
{

    /**
     */
    class NAU_KERNEL_EXPORT Event
    {
    public:
        enum class ResetMode
        {
            Auto,
            Manual
        };

        Event(const Event&) = delete;
        Event(Event&&) = delete;

        Event(ResetMode mode = ResetMode::Auto, bool signaled = false);

        Event& operator= (const Event&) = delete;
        Event& operator= (Event&&) = delete;

        ~Event();

        /**
            @brief Get the reset mode of this event. Can be 'Auto' or 'Manual'.
            @returns Reset mode for current event instance.
        */
        ResetMode getMode() const;

        /**
            @brief
                Sets the state of the event to signaled, allowing one or more waiting threads to proceed.
                For an event with reset mode = Auto , the set method releases a single thread. If there are no waiting threads, the wait handle remains signaled until a thread attempts to wait on it, or until its Reset method is called.
        */
        void set();

        void reset();

        /**
            @brief Blocks the current thread until the current WaitHandle receives a signal, using the timeout interval in milliseconds.
            @param[in] timeout timeout
            @returns true if the current event receives a signal; otherwise, false.
        */
        bool wait(std::optional<std::chrono::milliseconds> timeout = std::nullopt);

    protected:
        const ResetMode m_mode;
#if NAU_THREADING_EVENT_STDLIB
        std::mutex m_mutex;
        std::condition_variable m_signal;
        bool m_state;
#else
        void* const m_hEvent;
#endif
    };

}  // namespace nau::threading
