// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/threading/barrier.h"

#include <chrono>

#include "nau/diag/assertion.h"

namespace nau::threading
{
    Barrier::Barrier(size_t total) :
        m_total(total)
    {
        NAU_ASSERT(m_total > 0);
    }

    bool Barrier::enter(std::optional<std::chrono::milliseconds> timeout)
    {
        const size_t maxExpectedCounter = m_total - 1;
        const size_t counter = m_counter.fetch_add(1);

        NAU_ASSERT(counter < m_total);

        if (counter == maxExpectedCounter)
        {
            m_signal.notify_all();
            return true;
        }

        std::unique_lock lock(m_mutex);
        if (!timeout)
        {
            m_signal.wait(lock, [this]
            {
                return m_counter.load() == m_total;
            });
            return true;
        }

        return m_signal.wait_for(lock, std::chrono::milliseconds(timeout->count()), [this]
        {
            return m_counter.load() == m_total;
        });
    }

}  // namespace nau::threading
