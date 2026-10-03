// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "concurrent_execution_container.h"

#include <chrono>
#include <memory>

#include "nau/app/application.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/service/internal/service_provider_initialization.h"
#include "nau/service/service.h"
#include "nau/threading/set_thread_name.h"

namespace nau
{
    namespace
    {
        struct Timer
        {
            std::chrono::system_clock::time_point lastTimePoint = std::chrono::system_clock::now();

            std::chrono::milliseconds getDt()
            {
                using namespace std::chrono;

                const auto currentTimePoint = system_clock::now();
                const milliseconds dt = duration_cast<milliseconds>(currentTimePoint - lastTimePoint);
                lastTimePoint = currentTimePoint;
                return dt;
            }
        };
    }  // namespace

    ConcurrentExecutionContainer::ConcurrentExecutionContainer(IClassDescriptor::Ptr systemClass) :
        m_systemClass(std::move(systemClass))
    {
        NAU_FATAL(m_systemClass->getConstructor() != nullptr);
    }

    async::Task<> ConcurrentExecutionContainer::preInitService()
    {
        using namespace nau::async;

        m_thread = std::thread([this]() mutable
        {
            threading::setThisThreadName(::fmt::format("NAU SYS ({})", m_systemClass->getClassName()));

            m_workQueue = WorkQueue::create();
            Executor::setThisThreadExecutor(m_workQueue);

            TaskSource<> threadCompletedTaskSource;
            m_threadCompletion = threadCompletedTaskSource.getTask();

            scope_on_leave
            {
                threadCompletedTaskSource.resolve();
            };

            m_gameSystemInstance = *m_systemClass->getConstructor()->invoke(nullptr, {});
            NAU_FATAL(m_gameSystemInstance);
            NAU_FATAL(m_gameSystemInstance->is<IGameSceneUpdate>());

            getServiceProvider().addService(std::unique_ptr<IRttiObject>{m_gameSystemInstance});

            // If game system requires fixed time step update then WorkQueue::poll() will use blocking mode:
            // to maintain the required refresh rate, it may be necessary to skip some time - in this case,
            // the poll will block the thread (if there are no active tasks).
            //
            // If a fixed update frequency is not required, then the poll will work without thread blocking and time gaps,
            // ensuring the maximum speed of calling the game system update
            const std::optional<std::chrono::milliseconds> BlockingTimeout = std::nullopt;
            const std::optional<std::chrono::milliseconds> NonBlockingTimeout = std::chrono::milliseconds(0);

            for (m_executionTask = executeGameSystem(); !m_executionTask.isReady();)
            {
                const bool isFixedGameStep = m_gameSystemInstance->as<IGameSceneUpdate&>().getFixedUpdateTimeStep().has_value();
                m_workQueue->poll((isFixedGameStep && m_isAlive) ? BlockingTimeout : NonBlockingTimeout);
            }

            while (!m_isShutdownCompleted)
            {
                m_workQueue->poll(NonBlockingTimeout);
            }
        });

        // preInitCompletion will be resolved inside executeGameSystem
        // immediately after the initialization of the game system is completed within the its dedicated thread.
        return m_preInitCompletion.getTask();
    }

    async::Task<> ConcurrentExecutionContainer::initService()
    {
        NAU_FATAL(m_gameSystemInstance);

        if (IServiceInitialization* gameSystemInit = m_gameSystemInstance->as<IServiceInitialization*>())
        {
            co_await m_workQueue;
            co_await gameSystemInit->initService();
        }

        m_initCompletion.resolve();
    }

    async::Task<> ConcurrentExecutionContainer::shutdownService()
    {
        NAU_ASSERT(!m_isShutdownCompleted);
        NAU_FATAL(m_workQueue);

        m_isAlive = false;

        if (IServiceShutdown* gameSystemShutdown = m_gameSystemInstance->as<IServiceShutdown*>())
        {
            scope_on_leave
            {
                m_isShutdownCompleted = true;
                m_workQueue->notify();
            };

            co_await m_workQueue;
            co_await gameSystemShutdown->shutdownService();
        }
        else
        {
            m_workQueue->notify();
        }

        // There is need to switch out from current executor to awaiting thread completion:
        // - can not complete thread and poll queue at the same time: m_workQueue is polled inside this m_thread
        // - m_threadCompletion MUST be awaited only outside of m_workQueue executor (so switch to default pool to awaiting for thread completion).
        co_await async::Executor::getDefault();
        co_await m_threadCompletion;
        m_thread.join();
    }

    async::Task<> ConcurrentExecutionContainer::executeGameSystem()
    {
        using namespace std::chrono;
        using namespace nau::async;

        m_isAlive = true;

        if (auto* const gameSystemInit = m_gameSystemInstance->as<IServiceInitialization*>())
        {
            getServiceProvider().as<core_detail::IServiceProviderInitialization&>().setInitializationProxy(*gameSystemInit, this);

            co_await gameSystemInit->preInitService();
        }

        {
            m_preInitCompletion.resolve();
            co_await m_initCompletion.getTask();
        }

        IGameSceneUpdate& gameSceneUpdate = m_gameSystemInstance->as<IGameSceneUpdate&>();

        const auto syncSceneState = [&gameSceneUpdate]() -> Task<>
        {
            co_await getApplication().getExecutor();
            gameSceneUpdate.syncSceneState();
        };

        Timer timer;
        do
        {
            const std::chrono::milliseconds updateStep = timer.getDt();

            const bool doContinueUpdate = co_await gameSceneUpdate.update(updateStep);
            if (!doContinueUpdate)
            {
                m_workQueue->notify();
                break;
            }

            if (m_isAlive)
            {
                co_await syncSceneState();
            }
            else
            {
                // gameSceneUpdate.update can be finished in synchronous fashion.
                // but there is a need to always pump work queue - forcefully yield execution to polling async messages
                co_await m_workQueue;
            }

            // with fixed time step the game system will simulate for fixed time duration.
            // If the simulation calculation took less than the target time,
            // then in this case we simply fall asleep for a while to maintain the specified system update rate.
            //
            // f the simulation takes longer, then
            // 1) most likely the target update rate needs to be revised (but this can be done only by game system itself
            // 2) give control (yield) to the queue - so that there is an opportunity to pump the accumulated asynchronous messages
            //    and immediately proceed to the next step of the simulation
            if (const auto fixedTimeStep = gameSceneUpdate.getFixedUpdateTimeStep(); fixedTimeStep.has_value())
            {
                const auto updateDuration = duration_cast<milliseconds>(system_clock::now() - timer.lastTimePoint);

                if (updateDuration < *fixedTimeStep)
                {
                    const auto sleepTime = *fixedTimeStep - updateDuration;

                    // TODO: add async delay(timeout) -> Task<>.
                    // Or revise the implementation of the timer manager so as not to terminate the current coroutine with an error,
                    // but to immediately complete the co_await without waiting.
                    //
                    // Currently co_await timeout will automatically completed current coroutine.
                    // But to complete gracefully, the game system expects gameSceneUpdate to always be called
                    // otherwise it will never complete its execution: executeGameSystem will only stops after gameSceneUpdate.update() returns false.
                    //
                    // As a workaround, a "proxy" task is used to catch-up an error.
                    auto sleepTask = [&]() -> Task<>
                    {
                        co_await sleepTime;
                    }();

                    Result<> waitRes = co_await sleepTask.doTry();
                    if (!waitRes)
                    {
                        m_workQueue->notify();
                    }
                }
                else
                {
                    co_await m_workQueue;
                }
            }

        } while (true);
    }

}  // namespace nau
