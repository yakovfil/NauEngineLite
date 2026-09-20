// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <chrono>

#ifdef _MSC_VER
    #define NAU_STOPWATCH_EXPORT __declspec(dllexport)
#else
    #define NAU_STOPWATCH_EXPORT
#endif

namespace nau
{
    template <class TClock, class TDt>
    class NAU_STOPWATCH_EXPORT /* FIXME: SceneBaseSample DLL build */ Stopwatch
    {
        using TOutSeconds =
            std::chrono::duration<TDt, std::chrono::seconds::period>;

    public:
        using TTimePoint = TClock::time_point;
        using TDuration = TClock::duration;

        Stopwatch()
        {
            m_startPoint = TClock::now();
            tick();
        }

        TDuration getTime() const
        {
            return m_lastPoint - m_startPoint;
        }
        TDt getDtSeconds() const
        {
            return TOutSeconds(m_lastDt).count();
        }

        TDt tick()
        {
            auto now = TClock::now();
            m_lastDt = now - m_lastPoint;
            m_lastPoint = now;

            return getDtSeconds();
        }

    private:
        TTimePoint m_startPoint;
        TTimePoint m_lastPoint;
        TDuration m_lastDt;
    };

    class NAU_STOPWATCH_EXPORT /* FIXME : SceneBaseSample DLL build */ TickStopwatch
        : public Stopwatch<std::chrono::steady_clock, float>
    {
    };

}  // namespace nau

#undef NAU_STOPWATCH_EXPORT
