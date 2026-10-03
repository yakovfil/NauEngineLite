// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/test/helpers/stopwatch.h"

namespace nau::test
{

    Stopwatch::Stopwatch() :
        m_timePoint(std::chrono::system_clock::now())
    {
    }

    std::chrono::milliseconds Stopwatch::getTimePassed() const
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now() - m_timePoint);
    }

}  // namespace nau::test
