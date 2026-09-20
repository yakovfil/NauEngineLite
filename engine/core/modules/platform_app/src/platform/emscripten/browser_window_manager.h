// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include <condition_variable>
#include <deque>
#include <memory>

#include "nau/app/core_window_manager.h"
#include "nau/app/platform_window_status.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_component.h"

namespace nau
{
    struct BrowserWindowContext;

    class BrowserWindowManager final : public ICoreWindowManager,
                                       public IPlatformWindowInitialization,
                                       public IBrowserWindowStatus,
                                       public async::Executor,
                                       public IRuntimeComponent,
                                       public IDisposable
    {
        NAU_CLASS_(nau::BrowserWindowManager, ICoreWindowManager, IPlatformWindowInitialization, IBrowserWindowStatus, async::Executor, IRuntimeComponent, IDisposable)

    public:
        BrowserWindowManager();
        ~BrowserWindowManager();
        void bindToCurrentThread() override;
        IPlatformWindow& getActiveWindow() override;
        nau::Ptr<IPlatformWindow> createWindow(bool) override;
        async::Task<> windowReady() override;
        BrowserWindowState getBrowserState() const override;
        bool isDocumentVisible() const override;
        async::Executor::Ptr getExecutor() override;
        Result<> pumpMessageQueue(bool, std::optional<std::chrono::milliseconds>) override;
        void waitAnyActivity() noexcept override;
        void scheduleInvocation(Invocation) noexcept override;
        bool hasWorks() override;
        void dispose() override;

    private:
        std::shared_ptr<BrowserWindowContext> m_context;
        nau::Ptr<IPlatformWindow> m_window;
        async::TaskSource<> m_ready;
        std::thread::id m_thread;
        std::mutex m_mutex;
        std::condition_variable m_signal;
        std::deque<Invocation> m_queue;
        bool m_closing = false;
        bool m_cleanupComplete = false;
        bool m_closed = false;
        unsigned m_invoking = 0;
    };
}  // namespace nau
