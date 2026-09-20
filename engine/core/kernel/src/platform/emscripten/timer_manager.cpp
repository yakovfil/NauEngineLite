// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <EASTL/vector.h>

#include <condition_variable>
#include <thread>

#include "nau/async/async_timer.h"
#include "nau/async/task.h"
#include "nau/diag/common_errors.h"
#include "nau/memory/singleton_memop.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/runtime/internal/runtime_object_registry.h"

namespace nau::async
{
    class PortableTimerManager final : public ITimerManager,
                                       public IRuntimeComponent,
                                       public IDisposable
    {
        NAU_RTTI_CLASS(nau::async::PortableTimerManager, ITimerManager, IRuntimeComponent, IDisposable)
        NAU_DECLARE_SINGLETON_MEMOP(PortableTimerManager)

    public:
        PortableTimerManager() :
            m_registration(*this),
            m_worker([this]
        {
            run();
        })
        {
        }

        ~PortableTimerManager()
        {
            dispose();
            m_worker.join();
            NAU_ASSERT(m_pending == 0);
        }

        void executeAfter(std::chrono::milliseconds delay, Executor::Ptr executor, ExecuteAfterCallback callback, void* data) override
        {
            NAU_ASSERT(callback);
            if (!callback)
                return;
            Entry entry{Clock::now() + delay, 0, nullptr, callback, data, std::move(executor)};
            {
                std::lock_guard lock(m_mutex);
                ++m_pending;
                if (!m_disposed)
                {
                    m_entries.push_back(std::move(entry));
                    m_changed.notify_one();
                    return;
                }
            }
            complete(std::move(entry), true).detach();
        }

        InvokeAfterHandle invokeAfter(std::chrono::milliseconds delay, InvokeAfterCallback callback, void* data) override
        {
            NAU_ASSERT(callback);
            std::lock_guard lock(m_mutex);
            if (m_disposed || !callback)
                return 0;
            const auto id = m_nextId++;
            m_entries.push_back({Clock::now() + delay, id, callback, nullptr, data, nullptr});
            ++m_pending;
            m_changed.notify_one();
            return id;
        }

        void cancelInvokeAfter(InvokeAfterHandle id) override
        {
            if (!id)
                return;
            std::lock_guard lock(m_mutex);
            const auto it = eastl::find_if(m_entries.begin(), m_entries.end(), [id](const Entry& entry)
            {
                return entry.id == id;
            });
            if (it != m_entries.end())
            {
                m_entries.erase(it);
                --m_pending;
                m_changed.notify_one();
            }
            // An entry removed by the worker has committed to completion.
            // Never wait for its callback here: callbacks may cancel other timers.
        }

        void dispose() override
        {
            std::lock_guard lock(m_mutex);
            m_disposed = true;
            m_changed.notify_one();
        }

        bool hasWorks() override
        {
            std::lock_guard lock(m_mutex);
            return m_pending != 0 || (m_disposed && !m_workerExited);
        }

    private:
        using Clock = std::chrono::steady_clock;
        struct Entry
        {
            Clock::time_point deadline;
            InvokeAfterHandle id;
            InvokeAfterCallback invoke;
            ExecuteAfterCallback execute;
            void* data;
            Executor::Ptr executor;
        };

        Task<> complete(Entry entry, bool shutdown)
        {
            if (entry.execute)
            {
                Error::Ptr error;
                if (shutdown)
                    error = NauMakeErrorT(OperationCancelledError)("Timers subsystem is disposed");
                if (entry.executor)
                {
                    ASYNC_SWITCH_EXECUTOR(entry.executor)
                }
                entry.execute(std::move(error), entry.data);
            }
            else
            {
                // Match the native contract: shutdown invokes uncancelled callbacks.
                entry.invoke(entry.data);
            }
            std::lock_guard lock(m_mutex);
            --m_pending;
            co_return;
        }

        void run()
        {
            std::unique_lock lock(m_mutex);
            for (;;)
            {
                if (m_entries.empty())
                {
                    if (m_disposed)
                        break;
                    m_changed.wait(lock);
                    continue;
                }
                auto it = eastl::min_element(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b)
                {
                    return a.deadline < b.deadline;
                });
                if (!m_disposed && it->deadline > Clock::now())
                {
                    const auto deadline = it->deadline;
                    m_changed.wait_until(lock, deadline);
                    continue;
                }
                Entry entry = std::move(*it);
                m_entries.erase(it);
                const bool shutdown = m_disposed;
                lock.unlock();
                complete(std::move(entry), shutdown).detach();
                lock.lock();
            }
            m_workerExited = true;
        }

        std::mutex m_mutex;
        std::condition_variable m_changed;
        eastl::vector<Entry> m_entries;
        InvokeAfterHandle m_nextId = 1;
        size_t m_pending = 0;
        bool m_disposed = false;
        bool m_workerExited = false;
        RuntimeObjectRegistration m_registration;
        std::thread m_worker;
    };

    ITimerManager::Ptr ITimerManager::createDefault()
    {
        return eastl::make_unique<PortableTimerManager>();
    }
}  // namespace nau::async
