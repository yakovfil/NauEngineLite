// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <span>
#include <thread>
#include <type_traits>

#include "nau/async/cpp_coroutine.h"
#include "nau/diag/assertion.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/ptr.h"
#include "nau/rtti/rtti_object.h"
#include "nau/rtti/weak_ptr.h"
#include "nau/utils/functor.h"
#include "nau/utils/preprocessor.h"

namespace nau::async
{

    /**
     */
    class NAU_ABSTRACT_TYPE Executor : public virtual IRefCounted
    {
        NAU_INTERFACE(nau::async::Executor, IRefCounted)

    public:
        using Ptr = nau::Ptr<Executor>;
        using WeakPtr = nau::WeakPtr<Executor>;

        using Callback = void (*)(void* data1, void* data2) noexcept;

        struct NAU_KERNEL_EXPORT InvokeGuard
        {
            InvokeGuard(Executor& exec);
            InvokeGuard(const InvokeGuard&) = delete;
            InvokeGuard(InvokeGuard&&) = delete;

            ~InvokeGuard();

            Executor& executor;
            const std::thread::id threadId;
            InvokeGuard* const prev = nullptr;
        };

        class NAU_KERNEL_EXPORT Invocation
        {
        public:
            static Invocation fromCoroutine(CoroNs::coroutine_handle<> coroutine);

            Invocation() = default;

            Invocation(Callback, void* data1, void* data2);

            Invocation(Invocation&&);

            Invocation(const Invocation&) = delete;

            ~Invocation();

            Invocation& operator=(Invocation&&);

            Invocation& operator=(const Invocation&) = delete;

            explicit operator bool() const;

            void operator()();

        private:
            void reset();

            Callback m_callback = nullptr;
            void* m_callbackData1 = nullptr;
            void* m_callbackData2 = nullptr;
        };

        /**
         */
        NAU_KERNEL_EXPORT static Executor::Ptr getDefault();

        /**
         */
        NAU_KERNEL_EXPORT static Executor::Ptr getInvoked();

        /**
         */
        NAU_KERNEL_EXPORT static Executor::Ptr getThisThreadExecutor();

        /**
         */
        NAU_KERNEL_EXPORT static Executor::Ptr getCurrent();

        /**
         */
        NAU_KERNEL_EXPORT static void setDefault(Executor::Ptr);

        /**
         */
        NAU_KERNEL_EXPORT static void setThisThreadExecutor(Executor::Ptr executor);

        NAU_KERNEL_EXPORT static void setExecutorName(Executor::Ptr executor, std::string_view name);

        /**
         */
        NAU_KERNEL_EXPORT static Executor::Ptr findByName(std::string_view name);

        NAU_KERNEL_EXPORT static void finalize(Executor::Ptr&& executor);

        /**
         */
        NAU_KERNEL_EXPORT void execute(Invocation invocation) noexcept;

        NAU_KERNEL_EXPORT void execute(std::coroutine_handle<>) noexcept;

        NAU_KERNEL_EXPORT void execute(Callback, void* data1, void* data2 = nullptr) noexcept;

        virtual void waitAnyActivity() noexcept = 0;

    protected:
        NAU_KERNEL_EXPORT static void invoke(Executor&, Invocation) noexcept;

        NAU_KERNEL_EXPORT static void invoke(Executor&, std::span<Invocation> invocations) noexcept;

        virtual void scheduleInvocation(Invocation) noexcept = 0;
    };

    /*
     *
     */
    struct ExecutorAwaiter
    {
        Executor::Ptr executor;

        ExecutorAwaiter(Executor::Ptr exec) :
            executor(std::move(exec))
        {
            NAU_ASSERT(executor, "Executor must be specified");
        }

        constexpr bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> continuation) const
        {
            if (executor)
            {
                executor->execute(std::move(continuation));
            }
        }

        constexpr void await_resume() const noexcept
        {
        }
    };

    inline ExecutorAwaiter operator co_await(Executor::Ptr executor)
    {
        return {std::move(executor)};
    }

    inline ExecutorAwaiter operator co_await(Executor::WeakPtr executorWeakRef)
    {
        auto executor = executorWeakRef.acquire();
        NAU_ASSERT(executor, "Executor instance expired");

        return {std::move(executor)};
    }

}  // namespace nau::async

#define ASYNC_SWITCH_EXECUTOR(executorExpression)                          \
    {                                                                      \
        ::nau::async::Executor::Ptr executorVar = executorExpression;      \
        NAU_ASSERT(executorVar);                                           \
                                                                           \
        if (nau::async::Executor::getCurrent().get() != executorVar.get()) \
        {                                                                  \
            co_await executorVar;                                          \
        }                                                                  \
    }\
