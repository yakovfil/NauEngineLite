// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once
#include <optional>

#include "nau/async/task_base.h"
#include "nau/async/work_queue.h"
#include "nau/dispatch/class_descriptor.h"
#include "nau/service/service.h"
#include "nau/threading/event.h"

namespace nau
{
    class ConcurrentExecutionContainer : public IServiceInitialization,
                                         public IServiceShutdown
    {
        NAU_RTTI_CLASS(nau::ConcurrentExecutionContainer, IServiceInitialization, IServiceShutdown)

    public:
        ConcurrentExecutionContainer(IClassDescriptor::Ptr systemClass);

        async::Task<> preInitService() override;

    private:
        async::Task<> initService() override;
        async::Task<> shutdownService() override;
        async::Task<> executeGameSystem();

        const IClassDescriptor::Ptr m_systemClass;
        std::thread m_thread;
        WorkQueue::Ptr m_workQueue;
        async::TaskSource<> m_preInitCompletion;
        async::TaskSource<> m_initCompletion;
        async::Task<> m_threadCompletion;
        async::Task<> m_executionTask;
        std::atomic<bool> m_isAlive = false;
        std::atomic<bool> m_isShutdownCompleted = false;
        std::optional<threading::Event> m_initSignal;
        IRttiObject* m_gameSystemInstance = nullptr;
    };

}  // namespace nau
