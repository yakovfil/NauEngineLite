// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <gtest/gtest.h>

#include "nau/app/browser_runtime_timing.h"

TEST(BrowserTiming, QueueWakesDoNotAdvanceDeadline)
{
    nau::browser_detail::StepDeadline clock;
    clock.reset(1000);
    for (int now = 1000; now < 1100; now += 5)
    {
        EXPECT_FALSE(clock.due(now));
        EXPECT_EQ(clock.waitTime(now), 1100 - now);
    }
    EXPECT_TRUE(clock.due(1100));
    EXPECT_EQ(clock.elapsedAndAdvance(1100), 100);
    EXPECT_FALSE(clock.due(1100));
}

TEST(BrowserTiming, MissedDeadlinesDoNotCatchUpOrProduceOversizedElapsedTime)
{
    nau::browser_detail::StepDeadline clock;
    clock.reset(0);
    EXPECT_TRUE(clock.due(5000));
    EXPECT_EQ(clock.elapsedAndAdvance(5000), 0);
    EXPECT_FALSE(clock.due(5000));
    EXPECT_EQ(clock.waitTime(5000), 100);
    EXPECT_EQ(clock.elapsedAndAdvance(5120), 100);
    EXPECT_EQ(clock.waitTime(5120), 100);
}

TEST(BrowserTiming, VisibilityResetStartsANewInterval)
{
    nau::browser_detail::StepDeadline clock;
    clock.reset(0);
    clock.reset(60000);
    EXPECT_FALSE(clock.due(60000));
    EXPECT_TRUE(clock.due(60100));
    EXPECT_EQ(clock.elapsedAndAdvance(60100), 100);
}
