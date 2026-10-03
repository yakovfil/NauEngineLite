// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/async/thread_pool_executor.h"

#include <vector>

#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/runtime/internal/runtime_object_registry.h"
#include "nau/threading/set_thread_name.h"
#include "nau/utils/functor.h"
#include "nau/utils/scope_guard.h"

namespace nau::async
{
    namespace
    {
#ifdef _WIN32
        size_t getDefaultThreadsCount()
        {
            constexpr DWORD MinThreadsCount = 5;

            ::SYSTEM_INFO sysInfo;
            ::GetSystemInfo(&sysInfo);

            return static_cast<size_t>(std::max(sysInfo.dwNumberOfProcessors / 3, MinThreadsCount));
        }
#else
        size_t getDefaultThreadsCount()
        {
            return 5;
        }
#endif
    }  // namespace

    /**
     */
    class ThreadPoolExecutor final : public Executor,
                                     public IRuntimeComponent
    {
        NAU_CLASS_(nau::async::ThreadPoolExecutor, Executor, IRuntimeComponent)

    public:
        ThreadPoolExecutor(std::optional<size_t> threadsCount)
        {
            const size_t maxThreads = threadsCount ? *threadsCount : getDefaultThreadsCount();
            m_threads.reserve(maxThreads);

            for (size_t i = 0; i < maxThreads; ++i)
            {
                m_threads.emplace_back([](ThreadPoolExecutor& executor, size_t threadIndex)
                {
                    threading::setThisThreadName(std::format("Nau Pool-{}", threadIndex + 1));
                    executor.threadWork();
                }, std::ref(*this), i);
            }

            RuntimeObjectRegistration{nau::Ptr<>{this}}.setAutoRemove();

            void();
        }

        ~ThreadPoolExecutor()
        {
            join();
        }

    private:
        void scheduleInvocation(Invocation invocation) noexcept override
        {
            NAU_ASSERT(invocation);
            if (!invocation)
            {
                return;
            }

            m_taskCounter.fetch_add(1);

            const std::lock_guard lock{m_mutex};

            m_invocations.emplace_back(std::move(invocation));
            m_signal.notify_all();
        }

        void waitAnyActivity() noexcept override
        {
            using namespace std::chrono_literals;

            constexpr auto SleepTimeout = 2ms;

            while (m_taskCounter.load() > 0)
            {
                std::this_thread::sleep_for(SleepTimeout);
            }
        }

        bool hasWorks() override
        {
            return m_taskCounter.load() > 0;
        }

        Invocation getOrWaitNextInvocation()
        {
            std::unique_lock lock{m_mutex};

            Invocation invocation;

            do
            {
                if (!m_invocations.empty())
                {
                    auto head = m_invocations.begin();
                    invocation = std::move(*head);
                    eraseInvocation(head);
                }
                else if (m_isActive)
                {
                    m_signal.wait(lock);
                }
            } while (!invocation && m_isActive);

            return invocation;
        }

        void threadWork()
        {
            while (true)
            {
                auto invocation = getOrWaitNextInvocation();
                if (!invocation)
                {
                    break;
                }

                scope_on_leave
                {
                    NAU_ASSERT(m_taskCounter > 0);
                    m_taskCounter.fetch_sub(1);
                };

                const Executor::InvokeGuard guard{*this};
                Executor::invoke(*this, std::move(invocation));
            }
        }

        void join()
        {
            m_isActive = false;
            m_signal.notify_all();

            for (auto& t : m_threads)
            {
                t.join();
            }
        }

        void eraseInvocation(std::vector<Invocation>::iterator where)
        {
            if (auto& last = m_invocations.back(); &(*where) != &last)
            {
                *where = std::move(last);
            }

            const auto newSize = m_invocations.size() - 1;
            m_invocations.resize(newSize);
        }

        std::atomic_bool m_isActive{true};
        std::vector<Invocation> m_invocations;
        std::vector<std::thread> m_threads;
        std::mutex m_mutex;
        std::condition_variable m_signal;
        std::atomic_size_t m_taskCounter = 0;
    };

    Executor::Ptr createThreadPoolExecutor(std::optional<size_t> threadsCount)
    {
        return rtti::createInstance<ThreadPoolExecutor, Executor>(threadsCount);
    }

}  // namespace nau::async
