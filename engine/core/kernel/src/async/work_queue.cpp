// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/async/work_queue.h"

#include "nau/async/task_base.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/runtime/internal/runtime_object_registry.h"
#include "nau/threading/event.h"

namespace nau
{
    /**
     */
    class WorkQueueImpl final : public WorkQueue,
                                public IRuntimeComponent
    {
        NAU_CLASS_(nau::WorkQueueImpl, WorkQueue, IRuntimeComponent)

    public:
        WorkQueueImpl();

        ~WorkQueueImpl();

    private:
        async::Task<> waitForWork() override;

        void poll(std::optional<eastl::chrono::milliseconds> time = std::nullopt) override;

        void notify() override;

        void setWakeCallback(void (*callback)()) override
        {
            m_wakeCallback = callback;
        }

        bool hasWorks() override;

        void waitAnyActivity() noexcept override;

        void scheduleInvocation(Invocation) noexcept override;

        void setName(std::string name) override
        {
            m_name = std::move(name);
        }

        std::string getName() const override
        {
            return m_name;
        }

    private:
        void notifyInternal()
        {
            if (m_signal)
            {
                m_signal.resolve();
            }

            m_event.set();
            if (auto callback = m_wakeCallback.load())
            {
                callback();
            }
        }

        std::mutex m_mutex;
        eastl::vector<async::Executor::Invocation> m_invocations;
        async::TaskSource<> m_signal;
        std::atomic<bool> m_isPolled = false;
        std::atomic<bool> m_isNotified = false;
        std::atomic<void (*)()> m_wakeCallback = nullptr;

        threading::Event m_event{threading::Event::ResetMode::Manual};

        std::string m_name;
    };

    WorkQueueImpl::WorkQueueImpl()
    {
        RuntimeObjectRegistration{nau::Ptr<>{this}}.setAutoRemove();
    }

    WorkQueueImpl::~WorkQueueImpl()
    {
    }

    async::Task<> WorkQueueImpl::waitForWork()
    {
        using namespace nau::async;

        lock_(m_mutex);
        NAU_ASSERT(!m_isPolled);

        if (!m_invocations.empty())
        {
            return Task<>::makeResolved();
        }

        if (!m_signal || m_signal.isReady())
        {
            m_signal = {};
        }

        return m_signal.getTask();
    }

    void WorkQueueImpl::poll(std::optional<eastl::chrono::milliseconds> timeout)
    {
        using namespace nau::async;
        using namespace eastl::chrono;
        using namespace eastl::chrono_literals;

        using Timer = system_clock;

        NAU_ASSERT(!m_isPolled);
        m_isPolled = true;

        scope_on_leave
        {
            m_isNotified = false;
            m_isPolled = false;
        };

        const time_point timePoint = Timer::now();
        const auto timeIsOut = [&]() -> bool
        {
            if (!timeout)
            {
                return false;
            }

            const auto dt = duration_cast<milliseconds>(Timer::now() - timePoint);
            return timeout->count() < dt.count();
        };

        auto takeInvocations = [this](eastl::vector<Executor::Invocation>& invocations) mutable
        {
            lock_(m_mutex);
            if (m_signal && m_signal.isReady())
            {
                m_signal = nullptr;
            }

            if (!m_invocations.empty())
            {
                invocations.resize(m_invocations.size());
                std::move(m_invocations.begin(), m_invocations.end(), invocations.begin());
                m_invocations.clear();
            }
            else
            {
                invocations.clear();
            }

            m_event.reset();
        };

        eastl::vector<Executor::Invocation> invocations;

        do
        {
            for (takeInvocations(invocations); invocations.empty(); takeInvocations(invocations))
            {
                if (timeout)
                {
                    const auto dt = duration_cast<milliseconds>(Timer::now() - timePoint);
                    if (dt.count() < timeout->count())
                    {
                        const std::chrono::milliseconds waitEventTimeout(timeout->count() - dt.count());
                        [[maybe_unused]] const bool waitTimeouted = m_event.wait(waitEventTimeout);
                    }
                    else
                    {
                        break;
                    }
                }
                else
                {
                    [[maybe_unused]] const bool waitTimeouted = m_event.wait();
                }

                if (m_isNotified)
                {
                    break;
                }
            }

            if (!invocations.empty())
            {
                const Executor::InvokeGuard guard{*this};
                Executor::invoke(*this, {invocations.data(), invocations.size()});
            }

        } while (!timeIsOut() && !m_isNotified);
    }

    void WorkQueueImpl::notify()
    {
        lock_(m_mutex);
        m_isNotified = true;
        notifyInternal();
    }

    bool WorkQueueImpl::hasWorks()
    {
        lock_(m_mutex);
        return !m_invocations.empty() || m_isPolled;
    }

    void WorkQueueImpl::waitAnyActivity() noexcept
    {
    }

    void WorkQueueImpl::scheduleInvocation(Invocation invocation) noexcept
    {
        lock_(m_mutex);
        m_invocations.emplace_back(std::move(invocation));

        notifyInternal();
    }

    WorkQueue::Ptr WorkQueue::create()
    {
        return rtti::createInstance<WorkQueueImpl, WorkQueue>();
    }

}  // namespace nau
