// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/runtime/internal/runtime_state.h"

#include <iostream>

#include "nau/async/async_timer.h"
#include "nau/async/thread_pool_executor.h"
#include "nau/async/work_queue.h"
#include "nau/memory/singleton_memop.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/runtime/internal/runtime_object_registry.h"
#include "nau/utils/scope_guard.h"

namespace nau::async
{
    NAU_KERNEL_EXPORT void dumpAliveTasks();
    NAU_KERNEL_EXPORT bool hasAliveTasksWithCapturedExecutor();
}  // namespace nau::async

namespace nau
{
    /**
     */
    class RuntimeStateImpl final : public RuntimeState
    {
        NAU_DECLARE_SINGLETON_MEMOP(RuntimeStateImpl)
    public:
        RuntimeStateImpl()
        {
            using namespace nau::async;

            RuntimeObjectRegistry::setDefaultInstance();
            ITimerManager::setDefaultInstance();
            m_defaultAsyncExecutor = createThreadPoolExecutor();
            Executor::setDefault(m_defaultAsyncExecutor);
        }

        ~RuntimeStateImpl()
        {
            NAU_ASSERT(m_state == State::ShutdownCompleted || m_state == State::ShutdownNeedCompletion, "RuntimeState::shutdown() is not completely processed");
            if (m_state == State::ShutdownNeedCompletion)
            {
                completeShutdown();
            }
        }

        Functor<bool()> shutdown(bool doCompleteShutdown) override
        {
            if (m_state != State::Operable)
            {
                return []
                {
                    return false;
                };
            }

            m_state = State::ShutdownProcessed;

            RuntimeObjectRegistry::getInstance().visitObjects<IDisposable>([](eastl::span<IRttiObject*> objects)
            {
                for (auto& object : objects)
                {
                    if (claimRuntimeDisposal(*object))
                    {
                        object->as<IDisposable&>().dispose();
                    }
                }
            });

            m_shutdownStartTime = std::chrono::system_clock::now();
            m_shutdownTooLongWarningShowed = false;

            return [this, doCompleteShutdown]
            {
                return shutdownStep(doCompleteShutdown);
            };
        }

        void completeShutdown() override
        {
            using namespace nau::async;

            NAU_ASSERT(m_state == State::ShutdownNeedCompletion);
            scope_on_leave
            {
                m_state = State::ShutdownCompleted;
            };

            Executor::setDefault(nullptr);
            m_defaultAsyncExecutor.reset();

            ITimerManager::releaseInstance();
            RuntimeObjectRegistry::releaseInstance();
        }

    private:
        enum class State
        {
            Operable,
            ShutdownProcessed,
            ShutdownNeedCompletion,
            ShutdownCompleted
        };

        bool shutdownStep(bool doCompleteShutdown)
        {
            using namespace std::chrono;

            if (m_state != State::ShutdownProcessed)
            {
                return false;
            }

            constexpr auto ShutdownTooLongTimeout = seconds(3);

            bool hasPendingWorks = false;
            bool hasReferencedExecutors = false;
            bool hasReferencedNonExecutors = false;

            RuntimeObjectRegistry::getInstance().visitObjects<IRuntimeComponent>(
                [&](eastl::span<IRttiObject*> objects)
            {
                for (auto& object : objects)
                {
                    auto& runtimeComponent = object->as<IRuntimeComponent&>();
                    if (runtimeComponent.hasWorks())
                    {
                        hasPendingWorks = true;
                    }
                    else if (IRefCounted* const refCounted = object->as<IRefCounted*>(); refCounted)
                    {
                        constexpr size_t ExecutorExpectedRefs = 2;
                        constexpr size_t NonExecutorExpectedRefs = 1;

                        const bool isExecutor = refCounted->is<async::Executor>();
                        const size_t expectedRefsCount = isExecutor ? ExecutorExpectedRefs : NonExecutorExpectedRefs;
                        const size_t currentRefsCount = refCounted->getRefsCount();

                        NAU_ASSERT(currentRefsCount >= expectedRefsCount);

                        if (currentRefsCount > expectedRefsCount)
                        {
                            if (isExecutor)
                            {
                                hasReferencedExecutors = true;
                            }
                            else
                            {
                                hasReferencedNonExecutors = true;
                            }
                        }
                    }
                }
            });

            bool canCompleteShutdown = !(hasPendingWorks || hasReferencedExecutors || hasReferencedNonExecutors);
            if (!canCompleteShutdown && (!hasPendingWorks && !hasReferencedNonExecutors))
            {
                canCompleteShutdown = !async::hasAliveTasksWithCapturedExecutor();
                if (canCompleteShutdown)
                {
                    NAU_LOG_WARNING("The application will be forcefully completed, but there is still references to the executor. This could potentially be a source of problems.");
                }
            }

            if (canCompleteShutdown)
            {
                m_state = State::ShutdownNeedCompletion;
                if (doCompleteShutdown)
                {
                    completeShutdown();
                }
            }
            else if (duration_cast<seconds>(system_clock::now() - m_shutdownStartTime) > ShutdownTooLongTimeout)
            {
                if (!m_shutdownTooLongWarningShowed)
                {
                    m_shutdownTooLongWarningShowed = true;
                    NAU_LOG_WARNING("It appears that the application completion is blocked: either there is an unfinished task or some executor is still referenced");
                    async::dumpAliveTasks();
                }
            }

            return !canCompleteShutdown;
        }

        async::Executor::Ptr m_defaultAsyncExecutor;
        State m_state = State::Operable;
        std::chrono::system_clock::time_point m_shutdownStartTime;
        bool m_shutdownTooLongWarningShowed = false;
    };

    RuntimeState::Ptr RuntimeState::create()
    {
        return eastl::make_unique<RuntimeStateImpl>();
    }

}  // namespace nau
