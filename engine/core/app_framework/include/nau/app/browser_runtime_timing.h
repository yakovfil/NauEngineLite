// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <algorithm>
#include <cstdint>

namespace nau::browser_detail
{
    class StepDeadline
    {
    public:
        static constexpr int Interval = 100;
        void reset(int64_t now)
        {
            m_last = now;
            m_next = now + Interval;
        }
        bool due(int64_t now) const
        {
            return now >= m_next;
        }
        int elapsedAndAdvance(int64_t now)
        {
            const auto gap = now - m_last;
            const int elapsed = gap > 250 ? 0 : static_cast<int>(std::clamp<int64_t>(gap, 0, Interval));
            reset(now);
            return elapsed;
        }
        int waitTime(int64_t now) const
        {
            return static_cast<int>(std::clamp<int64_t>(m_next - now, 0, Interval));
        }

    private:
        int64_t m_last = 0;
        int64_t m_next = Interval;
    };
}  // namespace nau::browser_detail
