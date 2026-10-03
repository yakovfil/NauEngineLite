// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <chrono>
#include <memory>

#include "nau/async/cpp_coroutine.h"
#include "nau/async/executor.h"
#include "nau/diag/error.h"
#include "nau/kernel/kernel_config.h"
#include "nau/rtti/rtti_object.h"

namespace nau::async
{
    // template<typename>
    // class Task;

    /**

    */
    // NAU_KERNEL_EXPORT Task<void> delay(std::chrono::milliseconds timeout);

    /**
     */
    struct NAU_ABSTRACT_TYPE ITimerManager : virtual IRttiObject
    {
        NAU_INTERFACE(nau::async::ITimerManager, IRttiObject);

        using Ptr = std::unique_ptr<ITimerManager>;

        using InvokeAfterHandle = uint64_t;
        using InvokeAfterCallback = void (*)(void*) noexcept;
        using ExecuteAfterCallback = void (*)(Error::Ptr, void*) noexcept;

        NAU_KERNEL_EXPORT
        static void setInstance(ITimerManager::Ptr);

        NAU_KERNEL_EXPORT
        static ITimerManager& getInstance();

        NAU_KERNEL_EXPORT
        static bool hasInstance();

        NAU_KERNEL_EXPORT
        static ITimerManager::Ptr createDefault();

        static inline void setDefaultInstance()
        {
            setInstance(createDefault());
        }

        static inline void releaseInstance()
        {
            setInstance(nullptr);
        }

        virtual void executeAfter(std::chrono::milliseconds timeout, async::Executor::Ptr, ExecuteAfterCallback callback, void* callbackData) = 0;

        virtual InvokeAfterHandle invokeAfter(std::chrono::milliseconds timeout, InvokeAfterCallback, void*) = 0;

        virtual void cancelInvokeAfter(InvokeAfterHandle) = 0;
    };

    /**
        @brief
        Implement await with explicit timeout:
        co_await 100ms;
    */
    inline void executeAfter(std::chrono::milliseconds timeout, async::Executor::Ptr executor, ITimerManager::ExecuteAfterCallback callback, void* callbackData)
    {
        ITimerManager::getInstance().executeAfter(timeout, std::move(executor), callback, callbackData);
    }

    /**

    */
    inline auto invokeAfter(std::chrono::milliseconds timeout, ITimerManager::InvokeAfterCallback callback, void* data)
    {
        return ITimerManager::getInstance().invokeAfter(timeout, callback, data);
    }

    /**
     */
    inline void cancelInvokeAfter(ITimerManager::InvokeAfterHandle handle)
    {
        return ITimerManager::getInstance().cancelInvokeAfter(handle);
    }

}  // namespace nau::async
