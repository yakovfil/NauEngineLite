// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <type_traits>

#include "nau/async/task_base.h"
#include "nau/threading/lock_guard.h"
#include "nau/threading/spin_lock.h"

namespace nau::async
{

    /**
     */
    template <typename T = void>
    class MultiTaskSource : public CoreTaskPtr
    {
        static constexpr bool IsVoid = std::is_same_v<void, T>;
        static_assert(IsVoid || std::is_copy_constructible_v<T>, "Type expected to be void or copy constructible");

        using TaskClientData = async_detail::TaskClientData<T>;

    public:
        MultiTaskSource() :
            CoreTaskPtr(createCoreTask<TaskClientData>(nullptr))
        {
        }

        MultiTaskSource(std::nullptr_t) :
            CoreTaskPtr(nullptr)
        {
        }

        MultiTaskSource(const MultiTaskSource&) = delete;

        MultiTaskSource& operator=(std::nullptr_t)
        {
            reset();
            return *this;
        }

        MultiTaskSource& operator=(const MultiTaskSource&) = delete;

        void reset()
        {
            lock_(m_mutex);
            m_awaiters.clear();
            static_cast<CoreTaskPtr&>(*this) = nullptr;
        }

        void emplace()
        {
            lock_(m_mutex);
            m_awaiters.clear();
            static_cast<CoreTaskPtr&>(*this) = createCoreTask<TaskClientData>(nullptr);
        }

        bool isReady() const
        {
            lock_(m_mutex);
            NAU_ASSERT(static_cast<bool>(*this), "Invalid state");
            if (!static_cast<bool>(*this))
            {
                return false;
            }

            return getCoreTask().isReady();
        }

        template <typename... Args>
        requires(IsVoid || std::is_constructible_v<T, Args...>)
        bool resolve(Args&&... args)
        {
            NAU_ASSERT(static_cast<bool>(*this), "Invalid state");
            if (!static_cast<bool>(*this))
            {
                return false;
            }

            static_assert(!IsVoid || sizeof...(args) == 0);

            bool resolved = false;

            if constexpr (IsVoid)
            {
                resolved = getCoreTask().tryResolve();
            }
            else
            {
                resolved = getCoreTask().tryResolve([&](CoreTask::Rejector&) noexcept
                {
                    NAU_ASSERT(!getClientData().result);
                    getClientData().result.emplace(std::forward<Args>(args)...);
                });
            }

            if (!resolved)
            {
                return false;
            }

            lock_(m_mutex);

            scope_on_leave
            {
                if (m_autoResetOnReady)
                {
                    static_cast<CoreTaskPtr&>(*this) = nullptr;
                }
            };

            [[maybe_unused]] TaskClientData& data = getClientData();
            if constexpr (IsVoid)
            {
                for (TaskSource<T>& awaiter : m_awaiters)
                {
                    awaiter.resolve();
                }
            }
            else
            {
                if (!m_awaiters.empty())
                {
                    // In the case when the result no longer needs to be kept inside MultiTaskSource<>, its value can be given via move.
                    // But move can be applied only for last awaiter. In cases where there is only single awaiter, the move assignment (instead copy) becomes especially relevant.

                    const size_t lastIndex = m_awaiters.size() - 1;
                    for (size_t i = 0; i < lastIndex; ++i)
                    {
                        auto& awaiter = m_awaiters[i];
                        awaiter.resolve(*data.result);
                    }

                    auto& lastAwaiter = m_awaiters[lastIndex];
                    if (m_autoResetOnReady)
                    {
                        lastAwaiter.resolve(std::move(*data.result));
                    }
                    else
                    {
                        lastAwaiter.resolve(*data.result);
                    }
                }
            }

            m_awaiters.clear();

            return resolved;
        }

        bool reject(Error::Ptr error)
        {
            NAU_ASSERT(static_cast<bool>(*this), "Invalid state");
            if (!static_cast<bool>(*this))
            {
                return false;
            }

            const bool rejected = getCoreTask().tryRejectWithError(error);
            if (rejected)
            {
                lock_(m_mutex);
                scope_on_leave
                {
                    if (m_autoResetOnReady)
                    {
                        static_cast<CoreTaskPtr&>(*this) = nullptr;
                    }
                };

                for (TaskSource<T>& taskSource : m_awaiters)
                {
                    taskSource.reject(error);
                }

                m_awaiters.clear();
            }

            return rejected;
        }

        Task<T> getNextTask()
        {
            lock_(m_mutex);

            NAU_ASSERT(static_cast<bool>(*this), "Invalid state");
            if (!static_cast<bool>(*this))
            {
                return Task<T>::makeRejected(NauMakeError("Task source invalid state"));
            }

            if (!getCoreTask().isReady())
            {
                TaskSource<T>& awaiter = m_awaiters.emplace_back();
                return awaiter.getTask();
            }

            // inner task is ready:
            // just returns result (value or error).
            if (auto error = getCoreTask().getError())
            {
                return Task<T>::makeRejected(std::move(error));
            }

            if constexpr (IsVoid)
            {
                return Task<>::makeResolved();
            }
            else
            {
                TaskClientData& data = getClientData();
                NAU_FATAL(data.result);
                return Task<T>::makeResolved(*data.result);
            }
        }

        void setAutoResetOnReady(bool resetOnReady)
        {
            lock_(m_mutex);
            m_autoResetOnReady = resetOnReady;
        }

    private:
        using Mutex = threading::SpinLock;

        decltype(auto) getClientData()
        {
            void* const ptr = getCoreTask().getData();
            return *reinterpret_cast<TaskClientData*>(ptr);
        }

        decltype(auto) getClientData() const
        {
            const void* const ptr = getCoreTask().getData();
            return *reinterpret_cast<const TaskClientData*>(ptr);
        }

        std::vector<TaskSource<T>> m_awaiters;
        bool m_autoResetOnReady = false;
        mutable Mutex m_mutex;
    };

}  // namespace nau::async
