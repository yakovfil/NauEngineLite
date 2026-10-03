// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#include "helpers/runtime_guard.h"
#include "nau/test/helpers/stopwatch.h"
#include "nau/async/task.h"
#include "nau/async/work_queue.h"
#include "nau/threading/set_thread_name.h"

namespace nau::test
{
    using namespace ::testing;

    class TestAsyncFunctions : public ::testing::Test
    {
    protected:
        ~TestAsyncFunctions()
        {
            m_workQueue.reset();
            m_runtimeGuard.reset();
        }

        void runQueueThread()
        {
            m_queueThread = std::thread([this]
            {
                threading::setThisThreadName("Queue Thread");
                while(!m_isCompleted)
                {
                    m_workQueue->poll();
                }
            });
        }

        void setCompleted()
        {
            m_isCompleted = true;
        }

        RuntimeGuard::Ptr m_runtimeGuard = RuntimeGuard::create();
        WorkQueue::Ptr m_workQueue = WorkQueue::create();
        std::atomic<bool> m_isCompleted = false;
        std::thread m_queueThread;
    };

    /**
     */
    TEST_F(TestAsyncFunctions, NonBlockingWhenAll)
    {
        using namespace std::chrono_literals;
        using namespace nau::async;

        runQueueThread();

        const auto runBackgroundWork = []() -> Task<>
        {
            co_await Executor::getDefault();
            co_await 10ms;
        };

        const Stopwatch stopWatch;

        Task<> task = async::run([&]() -> Task<>
        {
            auto task1 = runBackgroundWork();
            auto task2 = runBackgroundWork();
            auto awaiter = async::whenAll(Expiration::never(), task1, task2);

            async::wait(awaiter);

            setCompleted();

            co_return;
        }, m_workQueue);

        m_queueThread.join();
        ASSERT_LE(stopWatch.getTimePassed().count(), 5000);
    }

    /**
     */
    TEST_F(TestAsyncFunctions, NonBlockingWhenAny)
    {
        runQueueThread();

        using namespace std::chrono_literals;
        using namespace nau::async;

        const auto runBackgroundWork = []() -> Task<>
        {
            co_await Executor::getDefault();
        };

        const Stopwatch stopWatch;

        Task<> task = async::run([&]() -> Task<>
        {
            auto task1 = runBackgroundWork().detach();
            auto task2 = runBackgroundWork().detach();
            auto awaiter = async::whenAny(Expiration::never(), task1, task2);

            async::wait(awaiter);

            setCompleted();

            co_return;
        }, m_workQueue);

        m_queueThread.join();
        ASSERT_LE(stopWatch.getTimePassed().count(), 5000);
    }

    /**
     */
    TEST_F(TestAsyncFunctions, Wait)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        {  // wait without timeout
            constexpr auto ThreadSleepTime = 40ms;

            auto task = async::run([ThreadSleepTime]() -> Task<>
            {
                co_await ThreadSleepTime;
            }, Executor::getDefault());

            const Stopwatch timer;
            ASSERT_TRUE(async::wait(task));
            ASSERT_TRUE(task.isReady());

            // TODO: currently actual timeout check is disabled (seems it does not works on CI).
            //ASSERT_THAT(timer.getTimePassed(), Gt(ThreadSleepTime - ThreadSleepTime / 10));  // Sometimes there are triggers in time with a difference of several milliseconds.
        }

        {  // wait with timeout
            constexpr auto TaskWaitTime = 2ms;
            constexpr auto ThreadSleepTime = 25ms;

            TaskSource<> signal;

            auto task = async::run([&]() -> Task<>
            {
                co_await ThreadSleepTime;
                co_await signal.getTask();
            }, Executor::getDefault());

            ASSERT_FALSE(async::wait(task, TaskWaitTime));
            ASSERT_FALSE(task.isReady());

            signal.resolve();

            ASSERT_TRUE(async::wait(task));
        }
    }

    /**
     */
    TEST_F(TestAsyncFunctions, AwaiterNonVoid)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        TaskSource<int> taskSource;

        const int Value = 10;

        auto awaiter = [](Task<int> task) -> Task<int>
        {
            const auto value = co_await std::move(task);
            co_return (value * 2);
        }(taskSource.getTask());

        [[maybe_unused]] auto t = async::run([&taskSource, Value]() -> Task<>
        {
            co_await 25ms;
            taskSource.resolve(Value);
        }, Executor::getDefault())
                                      .detach();

        async::wait(awaiter);

        EXPECT_THAT(awaiter.result(), Eq(Value * 2));
    }

    /**
     */
    TEST_F(TestAsyncFunctions, AwaiterVoid)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        TaskSource<> taskSource;

        auto awaiter = [](Task<> task) -> Task<>
        {
            co_await std::move(task);
            co_return;
        }(taskSource.getTask());

        [[maybe_unused]] auto t = async::run([&taskSource]() -> Task<>
        {
            co_await 25ms;
            taskSource.resolve();
        }, Executor::getDefault());

        async::wait(awaiter);
        async::wait(t);

        ASSERT_FALSE(awaiter.isRejected());
        ASSERT_TRUE(awaiter.isReady());
    }

    /**
     */
    TEST_F(TestAsyncFunctions, AwaiterRejectError)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        TaskSource<> taskSource;

        auto awaiter = [](Task<> task) -> Task<int>
        {
            co_await std::move(task);
            co_return 0;
        }(taskSource.getTask());

        [[maybe_unused]] auto t = async::run([&taskSource]() -> Task<>
        {
            co_await 25ms;
            taskSource.reject(NauMakeError("TestFailure"));
        }, Executor::getDefault());

        async::wait(awaiter);
        async::wait(t);

        ASSERT_TRUE(awaiter.isRejected());
        ASSERT_TRUE(awaiter.getError());
    }

    /**
      TEST:
  */
    TEST_F(TestAsyncFunctions, WhenAny_Container)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        using TaskSources = std::vector<TaskSource<>>;

        TaskSources taskSources(10);
        std::vector<Task<>> tasks;
        std::transform(taskSources.begin(), taskSources.end(), std::back_inserter(tasks), [](TaskSource<>& ts)
        {
            return ts.getTask().detach();
        });

        auto awaiter = async::run([](TaskSources taskSources) -> Task<>
        {
            co_await 10ms;
            for(TaskSource<>& ts : taskSources)
            {
                ts.resolve();
            }
        }, Executor::getDefault(), std::move(taskSources));

        async::wait(awaiter);

        const bool allIsReady = std::all_of(tasks.begin(), tasks.end(), [](const Task<>& t)
        {
            return t.isReady();
        });
        ASSERT_TRUE(allIsReady);
    }

    TEST_F(TestAsyncFunctions, WhenAny_Tasks)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        TaskSource<int> promise1;
        TaskSource<bool> promise2;
        TaskSource<std::string> promise3;

        auto task1 = promise1.getTask().detach();
        auto task2 = promise2.getTask().detach();
        auto task3 = promise3.getTask().detach();

        Task<bool> awaiter = async::whenAny(Expiration::never(), task1, task2, task3);

        [](auto& promise) -> Task<>
        {
            co_await 10ms;
            promise.resolve();
        }(promise2).detach();

        async::wait(awaiter);

        ASSERT_TRUE(awaiter.result());
        ASSERT_FALSE(task1.isReady());
        ASSERT_TRUE(task2.isReady());
        ASSERT_FALSE(task3.isReady());
    }

    /**
     */
    TEST_F(TestAsyncFunctions, WhenAll_Container)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t ChunksCount = 10;
        constexpr size_t TasksPerChunk = 100;
        constexpr size_t TasksCount = ChunksCount * TasksPerChunk;

        using TaskSources = std::vector<TaskSource<int>>;
        using ResolverFactory = Functor<Task<>()>;

        TaskSources taskSources(TasksCount);
        std::vector<Task<int>> tasks;
        std::transform(taskSources.begin(), taskSources.end(), std::back_inserter(tasks), [](auto& ts)
        {
            return ts.getTask().detach();
        });

        Task<bool> awaiter = async::whenAll(tasks);

        // tasks try to start as simultaneously as possible, to emulate resolving tasks in parallel
        std::list<ResolverFactory> resolveFactories;

        for(size_t i = 0; i < ChunksCount; ++i)
        {
            TaskSources chunkTaskSources(
                std::make_move_iterator(taskSources.begin()),
                std::make_move_iterator(taskSources.begin() + TasksPerChunk));

            taskSources.erase(taskSources.begin(), taskSources.begin() + TasksPerChunk);

            resolveFactories.emplace_back([chunkTaskSources = std::move(chunkTaskSources)]() mutable -> Task<>
            {
                return async::run([](TaskSources chunkTaskSources) -> Task<>
                {
                    size_t counter = 0;
                    for(auto& ts : chunkTaskSources)
                    {
                        if(++counter % 10 == 0)
                        {
                            co_await 1ms;
                        }
                        ts.resolve(10);
                    }
                }, Executor::getDefault(), std::move(chunkTaskSources));
            });
        }

        for(auto& f : resolveFactories)
        {
            [[maybe_unused]] auto t = f().detach();
        }

        const bool allIsReady = async::wait(awaiter);
        ASSERT_TRUE(allIsReady);
    }

    /**
     */
    TEST_F(TestAsyncFunctions, WhenAll_EmptyContainer)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        std::vector<Task<>> tasks;
        auto awaiter = whenAll(tasks);
        ASSERT_TRUE(awaiter.isReady());
        ASSERT_TRUE(awaiter.result());
    }

    /**
     */
    TEST_F(TestAsyncFunctions, WhenAll_AllReady)
    {
        using namespace nau::async;

        std::vector<Task<>> tasks;
        tasks.emplace_back(Task<>::makeResolved());
        tasks.emplace_back(Task<>::makeResolved());

        auto awaiter = whenAll(tasks);
        ASSERT_TRUE(awaiter.isReady());
        ASSERT_TRUE(awaiter.result());
    }

    /**
     */
    TEST_F(TestAsyncFunctions, WhenAll_Timeout)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        std::vector<TaskSource<>> taskSources;
        std::vector<Task<>> tasks;

        for(size_t i = 0; i < 10; ++i)
        {
            auto t = taskSources.emplace_back().getTask().detach();
            tasks.emplace_back(std::move(t));
        }

        const Stopwatch timer;
        const bool ready = *async::waitResult(whenAll(tasks, 15ms));

        ASSERT_FALSE(ready);
        ASSERT_THAT(timer.getTimePassed(), Gt(10ms));
    }

    /**
     */
    TEST_F(TestAsyncFunctions, WhenAll_NoTimeout)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        std::vector<TaskSource<>> taskSources;
        std::vector<Task<>> tasks;

        for(size_t i = 0; i < 10; ++i)
        {
            auto t = taskSources.emplace_back().getTask().detach();
            tasks.emplace_back(std::move(t));
        }

        [](auto& taskSources_) -> Task<>
        {
            co_await 10ms;
            for(auto& ts : taskSources_)
            {
                ts.resolve();
            }
        }(taskSources).detach();

        const Stopwatch timer;
        const bool ready = *async::waitResult(whenAll(tasks, 10000ms));

        ASSERT_TRUE(ready);
    }


  /*
        TEST: checking async::whenAny logic
            - one thread will randomly resolve TaskSource<>
            - another (main) thread waits for the result via Async::WhenAny.
            - added a small timeout in the resolve flow, so that it would be clear that whenAny did not work immediately, but at least after a specified timeout.
    */
    TEST_F(TestAsyncFunctions, WhenAny)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        m_runtimeGuard->reset();

        struct WhenAnyParam
        {
            const size_t totalTasks;
            const size_t expectedReadyTasks;

            WhenAnyParam(size_t total, size_t expected) :
                totalTasks(total),
                expectedReadyTasks(expected)
            {
            }
        };

        const std::array TestParams = {
            WhenAnyParam(100, 5),
            WhenAnyParam(10, 2),
            WhenAnyParam(10, 2),
            WhenAnyParam(5, 2),
            WhenAnyParam(5, 2)};

        for(const auto& param : TestParams)
        {
            const size_t TotalTasks = param.totalTasks;
            const size_t ExpectReadyTasksCount = param.expectedReadyTasks;
            NAU_ASSERT(ExpectReadyTasksCount <= TotalTasks);

            const auto runtimeGuard = RuntimeGuard::create();

            std::vector<TaskSource<>> taskSources(TotalTasks);
            std::vector<Task<>> tasks(taskSources.size());

            std::transform(taskSources.begin(), taskSources.end(), tasks.begin(), [](TaskSource<>& taskSource)
            {
                return taskSource.getTask().detach();
            });

            std::vector<size_t> readyIndices;

            constexpr auto ThreadSleepTime = 20ms;

            // taskSources cannot be passed inside theThread, because ~TaskSource<> always throws an unfinished task into error (and Task<>::isReady() will be true).
            std::thread theThread([&]() mutable
            {
                // suspending the thread to make sure (below) that whenAny is triggered no earlier than our ThreadSleepTime.
                std::this_thread::sleep_for(ThreadSleepTime);

                std::random_device rd{};
                std::default_random_engine re(rd());
                std::uniform_int_distribution<size_t> randomizer(0, taskSources.size() - 1);

                // randomly select the next free index.
                auto nextIndex = [&]() mutable
                {
                    constexpr size_t NotIndex = std::numeric_limits<size_t>::max();

                    size_t attempts = 5;
                    size_t index = NotIndex;

                    do
                    {
                        const size_t i = randomizer(re);
                        if(std::find(readyIndices.begin(), readyIndices.end(), i) == readyIndices.end())
                        {
                            index = i;
                        }

                    } while(index == NotIndex && (--attempts > 0));

                    if(index == NotIndex)
                    {  // if it was not possible to find a random one, any free one is selected
                        for(size_t i = 0, count = taskSources.size(); i < count; ++i)
                        {
                            if(std::find(readyIndices.begin(), readyIndices.end(), i) == readyIndices.end())
                            {
                                index = i;
                                break;
                            }
                        }
                    }

                    NAU_ASSERT(index != NotIndex);
                    return index;
                };

                do
                {
                    const size_t index = nextIndex();
                    auto& taskSource = taskSources[index];
                    if(taskSource.resolve())
                    {
                        readyIndices.push_back(index);
                    }
                } while(readyIndices.size() < ExpectReadyTasksCount);
            });

            const Stopwatch timer;
            // loop through several iterations to increase the likelihood that resolve and whenAny will be called simultaneously
            for(size_t repeat = 0; repeat < 10; ++repeat)
            {
                auto awaiter = async::whenAny(tasks);
                async::wait(awaiter);

                if(repeat == 0)
                {
                    ASSERT_THAT(timer.getTimePassed(), Ge(ThreadSleepTime));
                }
            }
            // 'join' here so that all indexes are filled.
            theThread.join();

            for(size_t index = 0; index < tasks.size(); ++index)
            {
                auto& task = tasks[index];
                const bool taskShouldBeReady = std::find(readyIndices.begin(), readyIndices.end(), index) != readyIndices.end();

                ASSERT_TRUE(task.isReady() == taskShouldBeReady);
            }

            ASSERT_EQ(ExpectReadyTasksCount, readyIndices.size());
        }
    };

    /*
        Test:
            checks the correct operation of the async::whenAny function when one of the tasks becomes completed while the work is running.
    */
    TEST_F(TestAsyncFunctions, WhenAny_TaskReady)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;


        constexpr size_t RepeatCount = 500;

        for(size_t i = 0; i < RepeatCount; ++i)
        {
            // Using signal1 increases the chances that resolve for a task will be called while whenAny is running - while whenAny has not yet returned the result (Task<bool>).
            // Under these conditions, whenAny should correctly return the completed task.
            TaskSource<> signal1;

            const size_t TasksCount = (i + 1) * 5;

            std::vector<TaskSource<>> sources(TasksCount);

            auto tasksAwaiter = async::run([&]() -> Task<>
            {
                std::vector<Task<>> tasks;
                tasks.reserve(sources.size());
                std::transform(sources.begin(), sources.end(), std::back_inserter(tasks), [](TaskSource<>& s)
                {
                    return s.getTask().detach();
                });

                signal1.resolve();

                co_await async::whenAny(sources);
            }, Executor::getDefault())
                                    .detach();

            auto tasksResolver = async::run([&]() -> Task<>
            {
                co_await signal1.getTask();

                for(auto& source : sources)
                {
                    source.resolve();
                }

                co_return;
            }, Executor::getDefault())
                                     .detach();

            // timeout allows you to complete the test if WhenAny was broken and the method got stuck
            auto timeout = []() -> Task<>
            {
                co_await 5s;
            }().detach();

            Task<bool> awaiter = async::whenAll(Expiration::never(), tasksResolver, tasksAwaiter).detach();

            [[maybe_unused]] const bool ok = *async::waitResult(async::whenAny(Expiration::never(), awaiter, timeout));

            ASSERT_FALSE(timeout.isReady());
        }
    };

    TEST_F(TestAsyncFunctions, WhenAny_WithTimeout)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        std::vector<TaskSource<>> tasksSources(10);
        std::vector<Task<>> tasks;
        std::transform(tasksSources.begin(), tasksSources.end(), std::back_inserter(tasks), [](TaskSource<>& s)
        {
            return s.getTask().detach();
        });

        constexpr auto Timeout = 50ms;

        const Stopwatch stopWatch;
        Task<bool> awaiter = async::whenAny(tasks, Timeout);
        async::wait(awaiter);

        ASSERT_FALSE(awaiter.result());
    };
}  // namespace nau::test
