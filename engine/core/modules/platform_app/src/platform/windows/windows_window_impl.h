#include <utility>
// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#pragma once

#include "nau/platform/windows/app/windows_window.h"
#include "nau/rtti/rtti_impl.h"

namespace nau
{
    struct IWindowManager;

    /**
     */
    class WindowsWindow final : public IWindowsWindow
    {
        NAU_CLASS_(nau::WindowsWindow, IWindowsWindow)

    public:
        WindowsWindow(IWindowManager&, HINSTANCE, const wchar_t* windowClassName, bool exitAppOnClose);

        ~WindowsWindow();

        IWindowManager& getWindowManager() const;

        bool exitAppOnClose() const;

        void destroyWindow();

        void setVisible(bool) override;

        bool isVisible() const override;

        std::pair<unsigned, unsigned> getSize() const override;
        std::pair<unsigned, unsigned> getClientSize() const override;

        HWND getWindowHandle() const override;

        void setSize(unsigned sizeX, unsigned sizeY) override;
        void setPosition(unsigned positionX, unsigned positionY) override;
        std::pair<unsigned, unsigned> getPosition() const override;
        void setName(const char* name) override;

    private:
        IWindowManager& m_windowManager;
        const bool m_exitAppOnClose;
        HWND m_hWnd = nullptr;
    };

}  // namespace nau
