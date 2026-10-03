#include <vector>
// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
// windows_app_impl.h

#pragma once

#include "./windows_window_impl.h"
#include "nau/app/core_window_manager.h"
#include "nau/platform/windows/app/window_message_handler.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/disposable.h"
#include "nau/runtime/internal/runtime_component.h"
#include "nau/service/service.h"

namespace nau
{
    class WindowsWindowManager final : public ICoreWindowManager,
                                       public async::Executor,
                                       public IRuntimeComponent,
                                       public IDisposable
    {
        NAU_CLASS_(nau::WindowsWindowManager,
                   ICoreWindowManager,
                   async::Executor,
                   IRuntimeComponent,
                   IDisposable)

    public:
        static constexpr const wchar_t* WindowClassName = L"NauDefaultWindowClass";

        WindowsWindowManager();

        ~WindowsWindowManager();

        IPlatformWindow& getActiveWindow() override;

        void bindToCurrentThread() override;

        async::Executor::Ptr getExecutor() override;

        Result<> pumpMessageQueue(bool waitForMessage, std::optional<std::chrono::milliseconds> maxProcessingTime) override;

        nau::Ptr<IPlatformWindow> createWindow(bool exitAppOnClose) override;

        void waitAnyActivity() noexcept override;

        void scheduleInvocation(Invocation) noexcept override;

        bool hasWorks() override;

        void dispose() override;

    private:
        static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        static nau::Ptr<WindowsWindow> createMainWindow(WindowsWindowManager*);

        bool checkAppThread();

        void processAsyncInvocations();

        bool handleWindowMessage(WindowsWindow& window, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        std::atomic<DWORD> m_threadId = 0;
        std::atomic<bool> m_isDisposed = false;
        nau::Ptr<WindowsWindow> m_window;

        std::vector<async::Executor::Invocation> m_asyncInvocations;
        std::vector<IWindowMessageHandler*> m_windowMessageHandlers;
        std::vector<IWindowsApplicationMessageHandler*> m_appMessageHandlers;
        std::mutex m_mutex;
        std::atomic<bool> m_isProcessAsyncInvocations = false;
    };
}  // namespace nau
