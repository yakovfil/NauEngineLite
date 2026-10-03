#include <utility>
// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "windows_window_impl.h"

#include NAU_PLATFORM_HEADER(diag/win_error.h)

namespace nau
{
    WindowsWindow::WindowsWindow(IWindowManager& windowManager, HINSTANCE hInstance, const wchar_t* windowClassName, bool exitAppOnClose) :
        m_windowManager(windowManager),
        m_exitAppOnClose(exitAppOnClose)
    {
        const DWORD style = WS_OVERLAPPEDWINDOW;

        m_hWnd = ::CreateWindowExW(0, windowClassName, L"NAU", style, 10, 10, 650, 650, nullptr, nullptr, hInstance, this);
        if (m_hWnd == nullptr)
        {
            const auto error = diag::getAndResetLastErrorCode();
            NAU_FAILURE("error_code ({}):", error, diag::getWinErrorMessageA(error));
        }
    }

    WindowsWindow::~WindowsWindow()
    {
        destroyWindow();
    }

    IWindowManager& WindowsWindow::getWindowManager() const
    {
        return m_windowManager;
    }

    bool WindowsWindow::exitAppOnClose() const
    {
        return m_exitAppOnClose;
    }

    void WindowsWindow::destroyWindow()
    {
        if (auto hWnd = std::exchange(m_hWnd, nullptr); hWnd != nullptr)
        {
            [[maybe_unused]] const BOOL destroyWindowOk = ::DestroyWindow(hWnd);
            if (destroyWindowOk != TRUE)
            {
                // Currently wil crash application on shutdown
                // NAU-2108
                // NAU_LOG_DEBUG("DestroyWindow returns not OK");
            }
        }
    }

    HWND WindowsWindow::getWindowHandle() const
    {
        return m_hWnd;
    }

    void WindowsWindow::setVisible(bool visible)
    {
        NAU_ASSERT(m_hWnd);

        const int flag = visible ? SW_SHOW : SW_HIDE;
        ::ShowWindow(m_hWnd, flag);
    }

    bool WindowsWindow::isVisible() const
    {
        return false;
    }

    std::pair<unsigned, unsigned> WindowsWindow::getSize() const
    {
        NAU_ASSERT(m_hWnd);

        RECT rect;

        [[maybe_unused]]
        const bool success = ::GetWindowRect(m_hWnd, &rect) == TRUE;
        NAU_ASSERT(success);
        NAU_ASSERT(rect.left <= rect.right);
        NAU_ASSERT(rect.top <= rect.bottom);

        return {rect.right - rect.left, rect.bottom - rect.top};
    }

    std::pair<unsigned, unsigned> WindowsWindow::getClientSize() const
    {
        NAU_ASSERT(m_hWnd);

        RECT rect;

        [[maybe_unused]]
        const bool success = ::GetClientRect(m_hWnd, &rect) == TRUE;
        NAU_ASSERT(success);
        NAU_ASSERT(rect.left <= rect.right);
        NAU_ASSERT(rect.top <= rect.bottom);

        return {rect.right - rect.left, rect.bottom - rect.top};
    }

    void WindowsWindow::setSize(unsigned sizeX, unsigned sizeY)
    {
        NAU_ASSERT(m_hWnd);
        ::SetWindowPos(m_hWnd, 0, 0, 0, sizeX, sizeY, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    void WindowsWindow::setPosition(unsigned positionX, unsigned positionY)
    {
        NAU_ASSERT(m_hWnd);
        ::SetWindowPos(m_hWnd, 0, positionX, positionY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
    }

    std::pair<unsigned, unsigned> WindowsWindow::getPosition() const
    {
        NAU_ASSERT(m_hWnd);

        RECT rect;

        [[maybe_unused]]
        const bool success = ::GetWindowRect(m_hWnd, &rect) == TRUE;
        NAU_ASSERT(success);

        return {rect.left, rect.top};
    }

    void WindowsWindow::setName(const char* name)
    {
        NAU_ASSERT(m_hWnd);
        ::SetWindowTextA(m_hWnd, name);
    }

}  // namespace nau
