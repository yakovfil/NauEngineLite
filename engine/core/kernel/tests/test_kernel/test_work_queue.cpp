// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <vector>

#include "nau/async/work_queue.h"
#include "nau/test/helpers/stopwatch.h"
#include "nau/threading/barrier.h"

namespace nau::test
{
    class TestWorkQueue : public ::testing::Test
    {
    protected:
        static async::Executor::Callback getDefaultCallback()
        {
            return [](void*, void*) noexcept
            {
            };
        }
    };

    TEST_F(TestWorkQueue, SimpleExecuteAndPoll)
    {
        auto queue = WorkQueue::create();

        bool flag = false;

        queue->execute([](void* data, void*) noexcept
        {
            *reinterpret_cast<bool*>(data) = true;
        }, &flag, nullptr);

        queue->poll();

        ASSERT_TRUE(flag);
    }

    TEST_F(TestWorkQueue, AwaitMultipleQueue)
    {
        auto queue1 = WorkQueue::create();
        auto queue2 = WorkQueue::create();

        std::vector<async::Task<>> awaiters;

        awaiters.push_back(queue1->waitForWork().detach());
        awaiters.push_back(queue2->waitForWork().detach());
        async::Task<bool> composedAwaiter = async::whenAny(awaiters);
        ASSERT_FALSE(composedAwaiter.isReady());

        queue1->execute(getDefaultCallback(), nullptr);
        ASSERT_TRUE(composedAwaiter.isReady());
    }

    TEST_F(TestWorkQueue, SimpleWait)
    {
        auto queue = WorkQueue::create();

        auto awaiter = queue->waitForWork();
        ASSERT_FALSE(awaiter.isReady());

        queue->execute(getDefaultCallback(), nullptr);

        ASSERT_TRUE(awaiter.isReady());
    }

    TEST_F(TestWorkQueue, NotifyAwaiter)
    {
        auto queue = WorkQueue::create();

        auto awaiter = queue->waitForWork();
        ASSERT_FALSE(awaiter.isReady());
        queue->notify();
        ASSERT_TRUE(awaiter.isReady());
    }

    TEST_F(TestWorkQueue, NotifyBeforeAwaiter)
    {
        auto queue = WorkQueue::create();
        queue->notify();

        auto awaiter = queue->waitForWork();
        ASSERT_FALSE(awaiter.isReady());
        awaiter.detach();
    }

    TEST_F(TestWorkQueue, WaitIsReadyAfterExecute)
    {
        auto queue = WorkQueue::create();
        queue->execute(getDefaultCallback(), nullptr);

        // specially called several times in a row
        ASSERT_TRUE(queue->waitForWork().isReady());
        ASSERT_TRUE(queue->waitForWork().isReady());
    }

    TEST_F(TestWorkQueue, Multithread)
    {
        using namespace nau::async;

        constexpr size_t ThreadsCount = 10;
        constexpr size_t ExecutePerThreadCount = 10'000;

        std::atomic_bool completed = false;

        auto queue = WorkQueue::create();

        std::thread pollThread([&]
        {
            while (!completed)
            {
                if (auto awaiter = queue->waitForWork(); !awaiter.isReady())
                {
                    async::wait(awaiter);
                }
                queue->poll();
            }
        });

        std::atomic_size_t counter = 0;

        auto work = [](void* counterPtr, void*) noexcept
        {
            reinterpret_cast<std::atomic_size_t*>(counterPtr)->fetch_add(1);
        };

        std::vector<std::thread> threads;
        threads.reserve(ThreadsCount);
        threading::Barrier barrier{ThreadsCount};

        for (size_t i = 0; i < ThreadsCount; ++i)
        {
            threads.emplace_back([&queue, &counter, work](threading::Barrier& barrier)
            {
                barrier.enter();
                for (size_t i = 0; i < ExecutePerThreadCount; ++i)
                {
                    queue->execute(work, &counter);
                }
            }, std::ref(barrier));
        }

        for (auto& t : threads)
        {
            t.join();
        }

        completed = true;
        queue->notify();
        pollThread.join();

        ASSERT_EQ(counter, ThreadsCount * ExecutePerThreadCount);
    }

    /**
        Test: poll with specified timeout.
        - execute poll with some time
        - check that poll is finished during specified timeout: the time spent inside the poll is greater than (or equal to) the timeout, but not significantly
     */
    TEST_F(TestWorkQueue, Timeout)
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        constexpr size_t IterCount = 3;
        constexpr auto Timeout = 10ms;

        auto queue = WorkQueue::create();

        for (auto i = 0; i < IterCount; ++i)
        {
            // more than one iteration is needed to verify queue state during multiple poll() calls
            const Stopwatch stopWatch;

            queue->poll(Timeout);

            ASSERT_GE(stopWatch.getTimePassed().count(), Timeout.count());

            // This may not work under debugger (when time is not limited).
            ASSERT_LT(stopWatch.getTimePassed().count(), Timeout.count() * 2);
        }
    }

    /**
        Test: thread-blocking poll without timeout it will be executed infinitely, but must be interrupted by WorkQueue::notify() (from other thread).
        - in main thread (blocking) poll(std::nullopt) is executed
        - starts the thread that must break poll by calling WorkQueue::notify() after some time of sleep
        - check that poll is interrupted during specified timeout: the time spent inside the poll is greater than (or equal to) the timeout, but not significantly
     */

    TEST_F(TestWorkQueue, BreakPollWithNotify)
    {
        using namespace std::chrono;
        using namespace std::chrono_literals;

        constexpr size_t IterCount = 3;
        constexpr auto Timeout = 10ms;

        auto queue = WorkQueue::create();

        for (auto i = 0; i < IterCount; ++i)
        {
            threading::Barrier barrier(2);

            std::thread notifyThread([&]
            {
                barrier.enter();
                std::this_thread::sleep_for(std::chrono::milliseconds(Timeout.count()));
                queue->notify();
            });

            barrier.enter();

            const Stopwatch stopWatch;

            queue->poll(std::nullopt);

            ASSERT_GE(stopWatch.getTimePassed().count(), Timeout.count());

            // This may not work under debugger (when time is not limited).
            ASSERT_LT(stopWatch.getTimePassed().count(), Timeout.count() * 2);

            notifyThread.join();
        }
    }

    /**
        Test: checks that poll actually executes the scheduled jobs within the specified timeout (without early exit).
        - start work thread where poll(WorkloadTimeout) will be called
        - call poll(WorkloadTimeout)
        - accumulate time actually spent inside the pool
        - from main thread execute jobs
        - check times spent inside the pool (is greater than (or equal to) the timeout)
        - check that jobs actually was executed

     */
    TEST_F(TestWorkQueue, WorkloadDuringTimeout)
    {
        using namespace std::chrono_literals;

        constexpr size_t IterCount = 3;
        constexpr auto WorkloadTimeout = 10ms;

        auto queue = WorkQueue::create();
        threading::Barrier barrier(2);

        std::atomic<bool> workThreadCompleted = false;
        std::atomic<size_t> index = 0;
        std::vector<std::chrono::milliseconds> workloadTimes;

        std::thread workThread([&]
        {
            scope_on_leave
            {
                workThreadCompleted = true;
            };

            barrier.enter();

            for (; index < IterCount; ++index)
            {
                const Stopwatch stopwatch;
                queue->poll(WorkloadTimeout);
                workloadTimes.push_back(stopwatch.getTimePassed());
            }
        });

        std::map<size_t, uint64_t> counters;

        barrier.enter();

        while (!workThreadCompleted)
        {
            queue->execute([](void* ptr, void* ptrIndex) noexcept
            {
                auto& data = *reinterpret_cast<decltype(counters)*>(ptr);
                auto& idx = *reinterpret_cast<decltype(index)*>(ptrIndex);

                data[idx] = data[idx] + 1;
            }, &counters, &index);
        }

        workThread.join();

        ASSERT_EQ(workloadTimes.size(), IterCount);
        ASSERT_EQ(counters.size(), IterCount);

        for (size_t i = 0; i < IterCount; ++i)
        {
            const auto workloadTime = workloadTimes[i];
            const auto workloadCounter = counters[i];

            ASSERT_GT(workloadCounter, 100);
            ASSERT_GE(workloadTime.count(), WorkloadTimeout.count());
            ASSERT_LT(workloadTime.count(), WorkloadTimeout.count() * 2);
        }
    }

}  // namespace nau::test
