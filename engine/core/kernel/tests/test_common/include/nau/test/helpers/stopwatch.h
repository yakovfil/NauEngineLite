// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once
#include <chrono>
#include <iostream>

namespace nau::test
{
    class Stopwatch
    {
    public:
        using CountT = decltype(std::chrono::milliseconds{}.count());

        Stopwatch();

        std::chrono::milliseconds getTimePassed() const;

    private:
        const std::chrono::system_clock::time_point m_timePoint;
    };

}  // namespace nau::test

namespace std::chrono
{
    inline void PrintTo(milliseconds t, ostream* os)
    {
        *os << t.count() << "ms";
    }

}  // namespace std::chrono
