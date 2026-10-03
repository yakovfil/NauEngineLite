// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.


#pragma once

#include <vector>

#include "./windows_window_impl.h"
#include "nau/app/platform_app.h"
#include "nau/platform/windows/app/window_message_handler.h"
#include "nau/rtti/rtti_impl.h"
#include "nau/runtime/internal/runtime_component.h"

namespace nau
{
    class WindowsApp final : public IPlatformApp,
                             public async::Executor,
                             public IRuntimeComponent
    {
        NAU_CLASS_(nau::WindowsApp, IPlatformApp, async::Executor, IRuntimeComponent)

    public:
        static constexpr const wchar_t* WindowClassName = L"NauDefaultWindowClass";

        WindowsApp();

        ~WindowsApp();

        void bindToCurrentThread() override;

        IPlatformWindow& getWindow() override;

        async::Executor::Ptr getExecutor() override;

        Result<> pumpMessageQueue(bool waitForMessage, std::optional<std::chrono::milliseconds> maxProcessingTime) override;

        void waitAnyActivity() noexcept override;

        void scheduleInvocation(Invocation) noexcept override;

        bool hasWorks() override;

        void dispose() override;

    private:
        static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        static nau::Ptr<WindowsWindow> createMainWindow(WindowsApp*);

        bool checkAppThread();

        void processAsyncInvocations();

        bool handleWindowMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

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
