// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <string>

#include "helpers/runtime_guard.h"
#include "nau/async/task.h"
#include "nau/math/transform.h"
#include "nau/test/helpers/assert_catcher_guard.h"
#include "nau/test/helpers/stopwatch.h"
#include "nau/threading/barrier.h"
#include "nau/threading/event.h"
#include "nau/utils/scope_guard.h"

namespace
{
    struct NonCopyable
    {
        std::unique_ptr<std::string> value;

        NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable(NonCopyable&&) = default;
    };

    struct DestructibleObject
    {
        virtual ~DestructibleObject() = default;
    };

    template <typename F>
    struct DestructibleObjectAction : DestructibleObject
    {
        F f;

        DestructibleObjectAction(F f_) :
            f(std::move(f_))
        {
        }

        ~DestructibleObjectAction()
        {
            f();
        }
    };

    struct alignas(16) CustomAlignedType16
    {
        uint64_t field1[55];
        nau::math::vec3 field2[23];
    };

    struct alignas(alignof(CustomAlignedType16) * 2) CustomAlignedType32
    {
        uint64_t field1[55];
        nau::math::vec3 field2[23];
    };

    template <typename F>
    std::shared_ptr<DestructibleObject> makeSharedDestructible(F&& f)
    {
        return std::make_shared<DestructibleObjectAction<F>>(std::forward<F>(f));
    }
}  // namespace

namespace nau::test
{
    using namespace ::testing;

    /**
        Test: Task<> state by default is invalid (stateless).
    */
    TEST(TestTask, StatelessByDefault)
    {
        using namespace nau::async;

        Task<> taskVoid;
        ASSERT_FALSE(taskVoid);

        Task<int> taskNonVoid;
        ASSERT_FALSE(taskNonVoid);
    }

    /**
        Test: TaskSource<> and Task<> is not copy constructible
    */
    TEST(TestTask, CheckNotCopyConstructible)
    {
        using namespace nau::async;

        static_assert(!std::is_copy_constructible_v<TaskSource<>>);
        static_assert(!std::is_copy_constructible_v<TaskSource<int>>);
        static_assert(!std::is_copy_constructible_v<Task<>>);
        static_assert(!std::is_copy_constructible_v<Task<int>>);
    }

    /**
        Test: TaskSource<> and Task<> is move constructible
    */
    TEST(TestTask, CheckTaskSourceMoveConstructible)
    {
        using namespace nau::async;

        static_assert(std::is_move_constructible_v<TaskSource<>>);
        static_assert(std::is_move_constructible_v<TaskSource<int>>);

        TaskSource<> taskVoidSource;
        ASSERT_TRUE(taskVoidSource);

        TaskSource<> taskVoidSource2 = std::move(taskVoidSource);

        ASSERT_TRUE(taskVoidSource2);
        ASSERT_FALSE(taskVoidSource);

        TaskSource<int> taskNonVoidSource;
        ASSERT_TRUE(taskNonVoidSource);

        auto taskNonVoidSource2 = std::move(taskNonVoidSource);

        ASSERT_TRUE(taskNonVoidSource2);
        ASSERT_FALSE(taskNonVoidSource);
    }

    /**
     */
    TEST(TestTask, MoveOnlyResult)
    {
        using namespace nau::async;

        using Container = std::vector<NonCopyable>;

        const auto factory = []() -> Task<Container>
        {
            Container values;
            values.emplace_back();

            co_return values;
        };

        auto task = [&]() -> Task<Container>
        {
            auto val = co_await factory();
            co_return val;
        }();

        async::wait(task);

        [[maybe_unused]] Container val = std::move(task).result();
    }

    /**
     */
    TEST(TestTask, MoveOnlyInRun)
    {
        using namespace nau::async;

        const auto runtimeGuard = RuntimeGuard::create();

        const auto factory = []() -> Task<NonCopyable>
        {
            NonCopyable value;
            co_return value;
        };

        auto task = async::run([&]
        {
            return factory();
        }, nullptr);

        async::wait(task);
        [[maybe_unused]] auto val = std::move(task).result();
    }

    /**
        Test:
            TaskSource<T>::Resolve()
    */
    TEST(TestTask, SimpleResolve)
    {
        using namespace nau::async;

        TaskSource<int> taskSource;
        taskSource.resolve(10);

        Task<int> task = taskSource.getTask();

        ASSERT_TRUE(task);
        ASSERT_TRUE(task.isReady());
        ASSERT_FALSE(task.isRejected());

        ASSERT_THAT(task.result(), Eq(10));
    }

    TEST(TestTask, SimpleResolveVoid)
    {
        using namespace nau::async;

        TaskSource<> taskSource;
        taskSource.resolve();

        Task<> task = taskSource.getTask();

        ASSERT_TRUE(task);
        ASSERT_TRUE(task.isReady());
        ASSERT_FALSE(task.isRejected());
    }

    /**
     */
    TEST(TestTask, SimpleRejectError)
    {
        using namespace nau::async;

        TaskSource<> taskSource;
        taskSource.reject(NauMakeError("Failure"));

        Task<> task = taskSource.getTask();

        ASSERT_TRUE(task);
        ASSERT_TRUE(task.isReady());
        ASSERT_TRUE(task.isRejected());
        ASSERT_TRUE(task.getError());
    }

    /**
     */
    TEST(TestTask, RejectReturnError)
    {
        using namespace nau::async;

        auto task = [&](bool doError) -> Task<int>
        {
            if (doError)
            {
                co_return NauMakeError("Failure");
            }

            co_return 0;
        }(true);

        ASSERT_TRUE(task);
        ASSERT_TRUE(task.isReady());
        ASSERT_TRUE(task.isRejected());
        ASSERT_TRUE(task.getError());
    }

    /**
     */
    TEST(TestTask, RejectYieldError)
    {
        using namespace nau::async;

        const auto getTask = [](bool doError, bool& leave, bool& called) -> Task<int>
        {
            leave = false;
            called = false;

            {
                scope_on_leave
                {
                    leave = true;
                };

                if (doError)
                {
                    co_yield NauMakeError("Failure");
                }
            }

            called = true;
            co_return 0;
        };

        bool leave = false;
        bool called = false;

        auto task = getTask(true, leave, called);

        ASSERT_TRUE(task.isReady());
        ASSERT_TRUE(task.isRejected());
        ASSERT_TRUE(task.getError());
        ASSERT_TRUE(leave);
        ASSERT_FALSE(called);
    }

    /**
        Test:  awaiting for Result<T>
            if the Result contains an error, then each operation fails with the same error, the co-program terminates (no code should be called after co_await), and local objects are destroyed correctly.
            if Result contains a value (i.e. not an error), then this value is returned as the result of the co_await operation, the co-program continues its execution.
    */
    TEST(TestTask, RejectAwaitResult)
    {
        using namespace nau::async;

        const auto getTask = [](bool doError, bool& leave, bool& called) -> Task<int>
        {
            leave = false;
            called = false;

            int value = 0;

            {
                scope_on_leave
                {
                    leave = true;
                };

                Result<int> res;

                if (doError)
                {
                    res = NauMakeError("Failure");
                }
                else
                {
                    res = 10;
                }

                value = co_await res;
            }
            called = true;
            co_return value;
        };

        constexpr bool DoError = true;

        {
            bool scopeLeave = false;
            bool wasCalled = false;
            auto task = getTask(DoError, scopeLeave, wasCalled);

            ASSERT_TRUE(scopeLeave);
            ASSERT_FALSE(wasCalled);
            ASSERT_TRUE(task);
            ASSERT_TRUE(task.isReady());
            ASSERT_TRUE(task.isRejected());
            ASSERT_TRUE(task.getError());
        }

        {
            bool scopeLeave = false;
            bool wasCalled = false;
            auto task = getTask(!DoError, scopeLeave, wasCalled);

            ASSERT_TRUE(scopeLeave);
            ASSERT_TRUE(wasCalled);
            ASSERT_TRUE(task);
            ASSERT_TRUE(task.isReady());
            ASSERT_FALSE(task.isRejected());
            ASSERT_FALSE(task.getError());
        }
    }

    /**
        Test:
            check for automatic termination of a coroutine if a nested co_await fails.
            In this case, the program must terminate through the chain of calls until the end or the first doTry().
    */
    TEST(TestTask, TaskErrorChain)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        const auto runtimeGuard = RuntimeGuard::create();

        const auto getResult = [](bool error, bool doAwait) -> Task<int>
        {
            if (doAwait)
            {
                co_await 5ms;
            }
            if (error)
            {
                co_yield NauMakeError("ErrorChainTest");
            }

            co_return 10;
        };

        const auto getTaskInner = [&](bool doError, bool doAwait, bool& leave, bool& called) -> Task<int>
        {
            // TODO: need to using other than default executor
            co_await Executor::getDefault();
            const Executor* const initialExecutor = Executor::getInvoked().get();

            int res = 0;

            {
                // the scope_on_leave is necessary to guarantee  to be executed before the co-program ends.
                // otherwise leave = true will be called after Promise::return_value, which will break the test conditions
                scope_on_leave
                {
                    leave = true;
                    const Executor* const finalExecutor = Executor::getInvoked().get();
                    NAU_ASSERT(finalExecutor == initialExecutor);
                };

                res = co_await getResult(doError, doAwait);
            }

            called = true;
            co_return res;
        };

        const auto getTaskOuter = [&](bool doError, bool doAwait, bool& leave1, bool& called1, bool& leave2, bool& called2) -> Task<int>
        {
            int res = 0;

            {
                // the scope_on_leave is necessary to guarantee  to be executed before the co-program ends.
                scope_on_leave
                {
                    leave1 = true;
                };

                auto t0 = getTaskInner(doError, doAwait, leave2, called2);
                res = co_await t0;
                called1 = true;
            }

            co_return res;
        };

        constexpr bool DoError = true;
        bool doAwait = false;

        // need to perform 4 call variations:  error + await, no error + await, error + no await, no error + no await
        do
        {
            {
                bool leave1 = false;
                bool leave2 = false;
                bool called1 = false;
                bool called2 = false;
                auto task = getTaskOuter(DoError, doAwait, leave1, called1, leave2, called2);
                async::wait(task);

                ASSERT_TRUE(leave1);
                ASSERT_TRUE(leave2);
                ASSERT_FALSE(called1);
                ASSERT_FALSE(called2);
                ASSERT_TRUE(task);
                ASSERT_TRUE(task.isReady());
                ASSERT_TRUE(task.isRejected());
                ASSERT_TRUE(task.getError());
                ASSERT_THAT(task.getError()->getMessage(), Eq("ErrorChainTest"));
            }

            {
                bool leave1 = false;
                bool leave2 = false;
                bool called1 = false;
                bool called2 = false;
                auto task = getTaskOuter(!DoError, doAwait, leave1, called1, leave2, called2);
                async::wait(task);

                ASSERT_TRUE(leave1);
                ASSERT_TRUE(leave2);
                ASSERT_TRUE(called1);
                ASSERT_TRUE(called2);
                ASSERT_TRUE(task);
                ASSERT_TRUE(task.isReady());
                ASSERT_FALSE(task.isRejected());
                ASSERT_FALSE(task.getError());
            }

            doAwait = !doAwait;
        } while (!doAwait);
    }

    /**
        Test: checking the functionality of Task<>::doTry()
            co_await for Task<T>::doTry() will return a Result<T>, which can be used to determine that the task completed with an error.
            In the case of doTry(), co_await always continues the current coroutine.
    */
    TEST(TestTask, TaskErrorTry)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        const auto runtimeGuard = RuntimeGuard::create();

        const auto getResult = [](bool error) -> Task<int>
        {
            co_await 5ms;
            if (error)
            {
                co_return NauMakeError("Failure");
            }

            co_return 10;
        };

        const auto getTask = [&](bool doError) -> Task<int>
        {
            const Result<int> res = co_await getResult(doError).doTry();
            co_return res;
        };

        constexpr bool DoError = true;

        {
            auto task = getTask(DoError);
            async::wait(task);

            ASSERT_TRUE(task.isRejected());
            ASSERT_TRUE(task.getError());
        }

        {
            auto task = getTask(!DoError);
            async::wait(task);

            ASSERT_FALSE(task.isRejected());
            ASSERT_THAT(*task, Eq(10));
        }
    }

    /**
    Test:
     TaskSource:
        > emplaceResult;
        > setException;
        > setReady;

     Task:
        > ready();
        > result();
        > rethrow();
        > exceptionPtr();



     1. Initialize task sources.
     2. Get tasks from sources.
     3. Run async operation to populate task sources with result: chooser random result or exception.
     4. Run async operations to wait while all tasks are ready.
     5. For all tasks check:
        > task ready;
        > task has value or exception;
        > for exceptional case check that 'result' is throw, 'rethrow' is throw, check exception type and exception data.
    */
    TEST(TestTask, ResolveNoException)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        const auto runtimeGuard = RuntimeGuard::create();

        constexpr size_t TaskCount = 2000;

        std::vector<TaskSource<size_t>> typedTaskSources(TaskCount);
        std::vector<TaskSource<>> voidTaskSources(TaskCount);

        std::vector<Task<size_t>> typedTasks;
        typedTasks.reserve(typedTaskSources.size());

        std::vector<Task<>> voidTasks;
        voidTasks.reserve(voidTaskSources.size());

        std::transform(typedTaskSources.begin(), typedTaskSources.end(), std::back_inserter(typedTasks), [](TaskSource<size_t>& source)
        {
            return source.getTask();
        });

        std::transform(voidTaskSources.begin(), voidTaskSources.end(), std::back_inserter(voidTasks), [](TaskSource<>& source)
        {
            return source.getTask();
        });

        for (const auto& task : typedTasks)
        {
            ASSERT_TRUE(task);
            ASSERT_FALSE(task.isReady());
        }

        for (const auto& task : voidTasks)
        {
            ASSERT_TRUE(task);
            ASSERT_FALSE(task.isReady());
        }

        const auto makeErrorMessage = [](size_t index) -> std::string
        {
            const std::string message = ::fmt::format("Error: {}", index);
            return std::string{message.data(), message.size()};
        };

        // Capturing by reference is possible in this case, because lambda is alive within the same scope as other variables
        Task<> t1 = async::run([&]() -> void
        {
            std::random_device rd;

            auto getRandomBoolean = [re = std::default_random_engine(rd()), randomizer = std::uniform_int_distribution<>{0, 1}]() mutable -> bool
            {
                return randomizer(re) != 0;
            };

            for (size_t index = 0; index < TaskCount; ++index)
            {
                if (getRandomBoolean())
                {
                    typedTaskSources[index].resolve(index);
                }
                else
                {
                    typedTaskSources[index].reject(NauMakeError(makeErrorMessage(index)));
                }

                if (getRandomBoolean())
                {
                    voidTaskSources[index].resolve();
                }
                else
                {
                    voidTaskSources[index].reject(NauMakeError(makeErrorMessage(index)));
                }
            }
        }, Executor::getDefault());

        Task<> t2 = async::run([&]() -> Task<>
        {
            const auto AllTasksReady = [](const auto& taskContainer)
            {
                return std::all_of(taskContainer.begin(), taskContainer.end(), [](auto& task)
                {
                    return task.isReady();
                });
            };

            while (!AllTasksReady(typedTasks))
            {
                co_await 1ms;
            }

            while (!AllTasksReady(voidTasks))
            {
                co_await 1ms;
            }
        }, Executor::getDefault());

        async::wait(t1);
        async::wait(t2);

        for (size_t i = 0; i < TaskCount; ++i)
        {
            auto& task = typedTasks[i];

            ASSERT_TRUE(task.isReady());

            if (auto err = task.getError())
            {
                ASSERT_THAT(makeErrorMessage(i), Eq(err->getMessage()));
            }
            else
            {
                ASSERT_EQ(i, task.result());
            }

            Task<>& voidTask = voidTasks[i];
            ASSERT_TRUE(voidTask.isReady());

            if (auto err = voidTask.getError())
            {
                ASSERT_THAT(makeErrorMessage(i), Eq(err->getMessage()));
            }
            else
            {
                voidTask.result();
            }
        }
    }

    /// <summary>
    /// Test:
    /// TaskSource:
    ///  > emplaceResult;
    ///  > setException;
    ///  > setReady;
    ///
    /// Task:
    ///  > ready();
    ///  > result();
    ///  > rethrow();
    ///  > exceptionPtr();
    //

    ///
    /// 1. Initialize task sources.
    /// 2. Get tasks from sources.
    /// 3. Run async operation to populate task sources with result: chooser random result or exception.
    /// 4. Run async operations to wait while all tasks are ready.
    /// 5. For all tasks check:
    ///  > task ready;
    ///  > task has value or exception;
    ///  > for exceptional case check that 'result' is throw, 'rethrow' is throw, check exception type and exception data.
    /// </summary>
    /// <param name=""></param>
    /// <param name=""></param>
    /// <returns></returns>
    TEST(TestTask, Resolve)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        const auto runtimeGuard = RuntimeGuard::create();

        constexpr size_t TaskCount = 2'000;

        std::vector<TaskSource<size_t>> typedTaskSources(TaskCount);
        std::vector<TaskSource<>> voidTaskSources(TaskCount);

        std::vector<Task<size_t>> typedTasks;
        typedTasks.reserve(typedTaskSources.size());

        std::vector<Task<>> voidTasks;
        voidTasks.reserve(voidTaskSources.size());

        std::transform(typedTaskSources.begin(), typedTaskSources.end(), std::back_inserter(typedTasks), [](TaskSource<size_t>& source)
        {
            return source.getTask();
        });

        std::transform(voidTaskSources.begin(), voidTaskSources.end(), std::back_inserter(voidTasks), [](auto& source)
        {
            return source.getTask();
        });

        for (const auto& task : typedTasks)
        {
            ASSERT_TRUE(task);
            ASSERT_FALSE(task.isReady());
        }

        for (const auto& task : voidTasks)
        {
            ASSERT_TRUE(task);
            ASSERT_FALSE(task.isReady());
        }

        const auto makeExceptionMessage = [](size_t index) -> std::string
        {
            const std::string message = fmt::format("Exception: {}", index);
            return std::string{message.data(), message.size()};
        };

        // Capturing by reference is possible in this case, because lambda is alive within the same scope as other variables
        Task<> t1 = async::run([&]() -> void
        {
            std::random_device rd;

            auto getRandomBoolean = [re = std::default_random_engine(rd()), randomizer = std::uniform_int_distribution<>{0, 1}]() mutable -> bool
            {
                return randomizer(re) != 0;
            };

            for (size_t index = 0; index < TaskCount; ++index)
            {
                if (getRandomBoolean())
                {
                    typedTaskSources[index].resolve(index);
                }
                else
                {
                    Error::Ptr error = NauMakeError(makeExceptionMessage(index));
                    typedTaskSources[index].reject(error);
                }

                if (getRandomBoolean())
                {
                    voidTaskSources[index].resolve();
                }
                else
                {
                    Error::Ptr error = NauMakeError(makeExceptionMessage(index));
                    voidTaskSources[index].reject(error);
                }
            }
        }, Executor::getDefault());

        Task<> t2 = async::run([&]() -> Task<>
        {
            const auto allTasksReady = [](const auto& taskContainer)
            {
                return std::all_of(taskContainer.begin(), taskContainer.end(), [](auto& task)
                {
                    return task.isReady();
                });
            };

            while (!allTasksReady(typedTasks))
            {
                co_await 1ms;
            }

            while (!allTasksReady(voidTasks))
            {
                co_await 1ms;
            }
        }, Executor::getDefault());

        async::wait(t1);
        async::wait(t2);

        for (size_t i = 0; i < TaskCount; ++i)
        {
            auto& task = typedTasks[i];

            ASSERT_TRUE(task.isReady());

            if (task.isRejected())
            {
                ASSERT_EQ(makeExceptionMessage(i), task.getError()->getMessage());
            }
            else
            {
                ASSERT_FALSE(task.isRejected());
                ASSERT_EQ(i, task.result());
            }

            auto& voidTask = voidTasks[i];
            ASSERT_TRUE(voidTask.isReady());

            if (voidTask.isRejected())
            {
                auto error = voidTask.getError();
                ASSERT_EQ(makeExceptionMessage(i), error->getMessage());
            }
            else
            {
                ASSERT_FALSE(voidTask.isRejected());
                voidTask.result();
            }
        }
    }

    /**
        Test:
            Checking the internal call CoreTask::setReadyCallback().
            Callback is set before resolve(). Called during resolve()
    */
    TEST(TestTask, CallbackBeforeResolve)
    {
        using namespace nau::async;

        TaskSource<> taskSource;
        Task<> task = taskSource.getTask();
        std::atomic<bool> flag = false;

        {
            CoreTaskPtr coreTask{task};
            getCoreTask(coreTask)->setReadyCallback([](void* ptr, void*) noexcept
            {
                reinterpret_cast<decltype(flag)*>(ptr)->store(true);
            }, &flag);
        }

        ASSERT_FALSE(flag);
        taskSource.resolve();
        ASSERT_TRUE(flag);
    }

    /**
        Test:
            Checking the internal call CoreTask::setReadyCallback().
            Callback is installed after resolve() is still called (at the time callback is installed).
    */
    TEST(TestTask, CallbackAfterResolve)
    {
        using namespace nau::async;

        TaskSource<> taskSource;
        Task<> task = taskSource.getTask();
        taskSource.resolve();

        std::atomic<bool> flag = false;

        {
            CoreTaskPtr coreTask{task};
            getCoreTask(coreTask)->setReadyCallback([](void* ptr, void*) noexcept
            {
                reinterpret_cast<decltype(flag)*>(ptr)->store(true);
            }, &flag);
        }

        ASSERT_TRUE(flag);
    }

    /**
        Test:
            Checking the internal call CoreTask::setReadyCallback().
            Stress mode: multiple parallel calls to resolve and setReadyCallback.
    */
    TEST(TestTask, CallbackStress)
    {
        using namespace nau::async;
        using namespace std::chrono_literals;

        constexpr size_t IterationsCount = 2'000;
        constexpr size_t TasksPerIteration = 100;

        const auto runtimeGuard = RuntimeGuard::create();

        std::vector<Task<>> workTasks;
        workTasks.reserve(IterationsCount * TasksPerIteration);

        std::atomic_size_t sourceCounter = 0;
        std::atomic_size_t taskCounter = 0;
        std::atomic_size_t callbackCounter = 0;

        const auto AllIsReady = [](const std::vector<Task<>>& tasks)
        {
            for (auto& t : tasks)
            {
                if (!t.isReady())
                {
                    return false;
                }
            }
            return true;
        };

        for (size_t i = 0; i < IterationsCount; ++i)
        {
            std::vector<TaskSource<>> sources;
            std::vector<Task<>> tasks;
            sources.reserve(TasksPerIteration);
            tasks.reserve(TasksPerIteration);

            for (size_t x = 0; x < TasksPerIteration; ++x)
            {
                tasks.emplace_back(sources.emplace_back().getTask());
            }

            workTasks.emplace_back([](auto allIsReady, std::atomic_size_t& counter, std::atomic_size_t& taskCounter, decltype(tasks) tasks) -> Task<>
            {
                co_await Executor::getDefault();

                for (auto& task : tasks)
                {
                    CoreTaskPtr coreTask{task};

                    getCoreTask(coreTask)->setReadyCallback([](void* ptr, void*) noexcept
                    {
                        reinterpret_cast<std::atomic_size_t*>(ptr)->fetch_add(1);
                    }, &counter);
                }

                while (!allIsReady(tasks))
                {
                    co_await 1ms;
                }
                taskCounter.fetch_add(tasks.size());
            }(AllIsReady, std::ref(callbackCounter), std::ref(taskCounter), std::move(tasks)));

            workTasks.emplace_back([](std::atomic_size_t& counter, decltype(sources) sources) -> Task<>
            {
                co_await Executor::getDefault();

                for (auto& taskSource : sources)
                {
                    taskSource.resolve();
                    counter.fetch_add(1);
                }
            }(std::ref(sourceCounter), std::move(sources)));
        }

        while (!AllIsReady(workTasks))
        {
            std::this_thread::yield();
            std::this_thread::sleep_for(1ms);
        }

        const size_t ExpectedCounterValue = IterationsCount * TasksPerIteration;
        ASSERT_THAT(sourceCounter, Eq(ExpectedCounterValue));
        ASSERT_THAT(taskCounter, Eq(ExpectedCounterValue));
        ASSERT_THAT(callbackCounter, Eq(ExpectedCounterValue));
    }

    TEST(TestTask, ResultDestruction)
    {
        using namespace nau::async;

        const auto runtimeGuard = RuntimeGuard::create();

        using DestructibleObjectPtr = std::shared_ptr<DestructibleObject>;

        auto task = []() -> Task<bool>
        {
            std::atomic_bool destructed = false;

            {
                auto innerTask = async::run([](std::atomic_bool& flag) -> Task<DestructibleObjectPtr>
                {
                    co_return makeSharedDestructible([&flag]
                    {
                        flag = true;
                    });
                }, {}, std::ref(destructed));

                [[maybe_unused]] auto temp = co_await innerTask;
            }

            co_return destructed.load();
        }();

        const bool success = *async::waitResult(std::ref(task));

        ASSERT_TRUE(success);
    }

    TEST(TestTask, ResultAlignment)
    {
        using namespace nau::async;

        static_assert(alignof(std::max_align_t) < alignof(CustomAlignedType16));
        static_assert(alignof(std::max_align_t) < alignof(CustomAlignedType32));

        AssertCatcherGuard assertGuard;

        for (size_t i = 0; i < 10; ++i)
        {
            TaskSource<CustomAlignedType16> taskSource16;
            Task<CustomAlignedType16> task16 = taskSource16.getTask();

            taskSource16.resolve(CustomAlignedType16{});
            [[maybe_unused]] auto result1 = *task16;

            TaskSource<CustomAlignedType32> taskSource32;
            Task<CustomAlignedType32> task32 = taskSource32.getTask();

            taskSource32.resolve(CustomAlignedType32{});
            [[maybe_unused]] auto result2 = *task32;
        }

        ASSERT_TRUE(assertGuard.assertFailureCounter == 0);
        ASSERT_TRUE(assertGuard.fatalFailureCounter == 0);
    }

}  // namespace nau::test
