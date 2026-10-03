// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <vector>

#include "helpers/runtime_guard.h"
#include "nau/async/multi_task_source.h"
#include "nau/async/task.h"
#include "nau/threading/barrier.h"

using namespace testing;
using namespace std::chrono_literals;

namespace nau::test
{

    /**
        Test: default state MultiTaskSource<> is valid.
    */
    TEST(TestMultiTaskSource, StatefulByDefault)
    {
        async::MultiTaskSource<void> ts;
        ASSERT_TRUE(ts);
    }

    /**
        Test: construct MultiTaskSource<> with no initial state.
    */
    TEST(TestMultiTaskSource, ConstructStateless)
    {
        async::MultiTaskSource<unsigned> ts = nullptr;
        ASSERT_FALSE(ts);
    }

    TEST(TestMultiTaskSource, Emplace)
    {
        async::MultiTaskSource<unsigned> ts = nullptr;
        ASSERT_FALSE(ts);
        ts.emplace();

        ASSERT_TRUE(ts);
    }

    /**
        Test: multiple getTask() + resolve()
    */
    TEST(TestMultiTaskSource, ResolveMulti)
    {
        using namespace async;

        constexpr unsigned ExpectedValue = 777;
        constexpr size_t TaskCount = 150;

        async::MultiTaskSource<unsigned> taskSource;

        std::vector<Task<unsigned>> tasks;
        tasks.reserve(TaskCount);

        for (size_t i = 0; i < TaskCount; ++i)
        {
            tasks.emplace_back(taskSource.getNextTask());
        }

        taskSource.resolve(ExpectedValue);
        for (auto& task : tasks)
        {
            ASSERT_TRUE(task.isReady());
            ASSERT_FALSE(task.isRejected());
            ASSERT_THAT(task.result(), Eq(ExpectedValue));
        }
    }

    /**
        Test: multiple get() + Reject()
    */
    TEST(TestMultiTaskSource, RejectMulti)
    {
        using namespace async;

        constexpr size_t TaskCount = 150;

        MultiTaskSource<> taskSource;

        std::vector<Task<>> tasks;
        tasks.reserve(TaskCount);

        for (size_t i = 0; i < TaskCount; ++i)
        {
            tasks.emplace_back(taskSource.getNextTask());
        }

        taskSource.reject(NauMakeError("test fail"));
        for (auto& task : tasks)
        {
            ASSERT_TRUE(task.isReady());
            ASSERT_TRUE(task.isRejected());
        }
    }

    /**
        Test:
        after MultiTaskSource is resolved all subsequent getTask() must returns completed/ready task
    */
    TEST(TestMultiTaskSource, GetResolved)
    {
        async::MultiTaskSource<std::string> taskSource;
        taskSource.resolve("test");

        auto t0 = taskSource.getNextTask();
        ASSERT_TRUE(t0.isReady());
        ASSERT_THAT(t0.result(), Eq("test"));

        auto t1 = taskSource.getNextTask();
        ASSERT_TRUE(t1.isReady());
        ASSERT_THAT(t1.result(), Eq("test"));
    }

    /**
        Test:
        after MultiTaskSource is rejected all subsequent getTask() must returns completed/ready task (with error)
    */

    TEST(TestMultiTaskSource, GetRejected)
    {
        async::MultiTaskSource<std::string> taskSource;
        taskSource.reject(NauMakeError("test"));

        auto t0 = taskSource.getNextTask();
        ASSERT_TRUE(t0.isReady());
        ASSERT_THAT(t0.getError(), NotNull());

        auto t1 = taskSource.getNextTask();
        ASSERT_TRUE(t1.isReady());
        ASSERT_THAT(t1.getError(), NotNull());
    }

    /**
        Test:
        all child tasks are going to be automaticaly completed when parent MultiTaskSource<> are destructed
    */
    TEST(TestMultiTaskSource, AutoRejectedOnDestruct)
    {
        async::MultiTaskSource<std::string> taskSource;

        auto t0 = taskSource.getNextTask();
        auto t1 = taskSource.getNextTask();

        taskSource = nullptr;

        ASSERT_THAT(t0.getError(), NotNull());
        ASSERT_THAT(t1.getError(), NotNull());
    }

    /**
        Test:
        Calling MultiTaskSource<>::resolve() make sense only once (any subsequent call will do nothing).
    */
    TEST(TestMultiTaskSource, CanResolveOnce)
    {
        async::MultiTaskSource<std::string> taskSource;
        auto t0 = taskSource.getNextTask();

        ASSERT_TRUE(taskSource.resolve("test1"));
        ASSERT_FALSE(taskSource.resolve("test2"));
        ASSERT_FALSE(taskSource.reject(NauMakeError("error")));

        auto t1 = taskSource.getNextTask();
        ASSERT_THAT(t0.result(), Eq("test1"));
        ASSERT_THAT(t1.result(), Eq("test1"));
    }

    /**
        Test:
        Calling MultiTaskSource<>::reject() make sense only once (any subsequent call will do nothing).
    */
    TEST(TestMultiTaskSource, CanRejectOnce)
    {
        async::MultiTaskSource<std::string> taskSource;
        auto t0 = taskSource.getNextTask();

        ASSERT_TRUE(taskSource.reject(NauMakeError("fail-1")));
        ASSERT_FALSE(taskSource.reject(NauMakeError("fail-2")));
        ASSERT_FALSE(taskSource.resolve("test2"));

        auto t1 = taskSource.getNextTask();

        ASSERT_TRUE(t0.isReady() && t0.isRejected());
        ASSERT_TRUE(t1.isReady() && t1.isRejected());
    }

    /**
        Test:
        MultiTaskSource<> multithreaded access
    */
    TEST(TestMultiTaskSource, MultiThreadGetNextTask)
    {
        using namespace async;

        constexpr size_t TaskCount = 1000;

        const RuntimeGuard::Ptr runtime = RuntimeGuard::create();

        MultiTaskSource<std::string> taskSource;
        TaskSource<> signalSource;
        std::atomic<size_t> counter = 0;

        std::vector<Task<>> tasks;
        tasks.reserve(TaskCount);

        for (size_t i = 0; i < TaskCount; ++i)
        {
            tasks.emplace_back([](MultiTaskSource<std::string>& multiTaskSource, TaskSource<>& signal, std::atomic<size_t>& taskCounter) -> Task<>
            {
                co_await Executor::getDefault();

                if (++taskCounter > TaskCount / 2)
                {
                    signal.resolve();
                }

                const std::string value = co_await multiTaskSource.getNextTask();
                if (value != "test")
                {
                    co_yield NauMakeError("Invalid value");
                }
            }(std::ref(taskSource), std::ref(signalSource), counter));
        }

        waitResult([](MultiTaskSource<std::string>& multiTaskSource, Task<> signal) -> Task<>
        {
            co_await signal;
            multiTaskSource.resolve("test");
        }(taskSource, signalSource.getTask()))
            .ignore();

        waitResult(whenAll(tasks)).ignore();

        for (auto& task : tasks)
        {
            ASSERT_TRUE(task.isReady());
            ASSERT_FALSE(task.isRejected());
        }
    }

    TEST(TestMultiTaskSource, MultiThreadResolve)
    {
        using namespace async;

        constexpr size_t TaskCount = 1000;

        const RuntimeGuard::Ptr runtime = RuntimeGuard::create();

        MultiTaskSource<std::string> taskSource;

        std::vector<Task<bool>> tasks;
        tasks.reserve(TaskCount);

        for (size_t i = 0; i < TaskCount; ++i)
        {
            tasks.emplace_back([](MultiTaskSource<std::string>& signalSource, size_t val) -> Task<bool>
            {
                co_await 1ms;
                co_return signalSource.resolve(fmt::format("result-{}", val));
            }(std::ref(taskSource), i));
        }

        async::waitResult([](MultiTaskSource<std::string>& signal) -> Task<>
        {
            co_await 5ms;
            signal.resolve("test");
        }(std::ref(taskSource)))
            .ignore();

        async::waitResult(whenAll(tasks)).ignore();

        const size_t resolveCounter = std::reduce(tasks.begin(), tasks.end(), size_t{0}, [](size_t accum, const Task<bool>& task)
        {
            return accum + (task.result() ? 1 : 0);
        });

        ASSERT_THAT(resolveCounter, Eq(1));
    }

    /**
        Test:
            If setAutoResetOnReady is set, then taskSource will reset its inner state
            when it will resolved (i.e. result's data will be released immediately)
    */
    TEST(TestMultiTaskSource, Resolve_AutoResetOnReady)
    {
        const std::string ExpectedResult = "test1";

        async::MultiTaskSource<std::string> taskSource;
        taskSource.setAutoResetOnReady(true);
        auto t0 = taskSource.getNextTask();
        auto t1 = taskSource.getNextTask();

        ASSERT_TRUE(taskSource.resolve(ExpectedResult));

        ASSERT_THAT(t0.result(), Eq(ExpectedResult));
        ASSERT_THAT(t1.result(), Eq(ExpectedResult));
        ASSERT_FALSE(taskSource);
    }

    TEST(TestMultiTaskSource, Resolve_AutoResetOnReadyVoid)
    {
        async::MultiTaskSource<> taskSource;
        taskSource.setAutoResetOnReady(true);
        auto t0 = taskSource.getNextTask();
        auto t1 = taskSource.getNextTask();

        ASSERT_TRUE(taskSource.resolve());

        ASSERT_TRUE(t0.isReady() && !t0.isRejected());
        ASSERT_TRUE(t1.isReady() && !t1.isRejected());
        ASSERT_FALSE(taskSource);
    }

    /**
        Test:
            If setAutoResetOnReady is set, then taskSource will reset its inner state
            when it will rejected (i.e. result's data will be released immediately)
    */
    TEST(TestMultiTaskSource, Reject_AutoResetOnReady)
    {
        const std::string ExpectedResult = "test1";

        async::MultiTaskSource<std::string> taskSource;
        taskSource.setAutoResetOnReady(true);
        auto t0 = taskSource.getNextTask();
        auto t1 = taskSource.getNextTask();

        ASSERT_TRUE(taskSource.reject(NauMakeError("error")));

        ASSERT_TRUE(t0.isRejected());
        ASSERT_TRUE(t1.isRejected());
        ASSERT_FALSE(taskSource);
    }

    /**
        Test:
            There is no problem to resolve MultiTaskSource
            when setAutoResetOnReady is set and no actual awaiters.
    */

    TEST(TestMultiTaskSource, Resolve_AutoResetOnReadyNoAwaiters)
    {
        const std::string ExpectedResult = "test1";

        async::MultiTaskSource<std::string> taskSource;
        taskSource.setAutoResetOnReady(true);
        ASSERT_TRUE(taskSource.resolve(ExpectedResult));
        ASSERT_FALSE(taskSource);
    }

    TEST(TestMultiTaskSource, Reject_AutoResetOnReadyNoAwaiters)
    {
        async::MultiTaskSource<std::string> taskSource;
        taskSource.setAutoResetOnReady(true);
        ASSERT_TRUE(taskSource.reject(NauMakeError("test")));
        ASSERT_FALSE(taskSource);
    }

}  // namespace nau::test
