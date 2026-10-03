// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <gtest/gtest.h>

#include <vector>

#include "nau/3d/dag_drv3d.h"
#include "nau/diag/logging.h"
#include "nau/osApiWrappers/dag_cpuJobs.h"
#include "nau/platform/windows/windows_headers.h"
#include "nau/service/service_provider.h"
#include "nau/startup/dag_globalSettings.h"
namespace
{
    struct InitCallback : Driver3dInitCallback
    {
        int validateDesc(Driver3dDesc&) const override
        {
            return 1;
        }
        int compareDesc(Driver3dDesc&, Driver3dDesc&) const override
        {
            return 0;
        }
        void verifyResolutionSettings(int& width, int& height, int, int, bool) const override
        {
            width = 320;
            height = 240;
        }
    };
    class Dx12Swapchains : public testing::Test
    {
    protected:
        std::vector<HWND> windows;
        bool initialized = false;
        HWND makeWindow()
        {
            auto hwnd = CreateWindowExW(0, L"STATIC", L"Nau migration swapchain probe", WS_OVERLAPPEDWINDOW,
                                        0, 0, 320, 240, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            if (hwnd)
                windows.push_back(hwnd);
            return hwnd;
        }
        void TearDown() override
        {
            if (initialized)
            {
                d3d::finish_render_commands();
                d3d::release_driver();
            }
            for (auto hwnd : windows)
                DestroyWindow(hwnd);
        }
    };
}  // namespace
TEST_F(Dx12Swapchains, CreatesDestroysAndRecreatesAdditionalWindows)
{
    HWND primary = makeWindow();
    ASSERT_NE(primary, nullptr);
    dgs_set_window_mode(WindowMode::WINDOWED);
    ASSERT_TRUE(d3d::init_driver());
    InitCallback callback;
    void* mainWindow = primary;
    ASSERT_TRUE(d3d::init_video(GetModuleHandleW(nullptr), nullptr, "STATIC", SW_HIDE, mainWindow,
                                primary, nullptr, "Nau migration swapchain probe", &callback));
    initialized = true;
    ASSERT_NE(d3d::get_back_buffer_rt(DEFAULT_SWAPID), nullptr);
    for (int cycle = 0; cycle < 3; ++cycle)
    {
        HWND first = makeWindow(), second = makeWindow();
        ASSERT_NE(first, nullptr);
        ASSERT_NE(second, nullptr);
        const SWAPID a = d3d::create_swapchain(first), b = d3d::create_swapchain(second);
        ASSERT_NE(a, DEFAULT_SWAPID);
        ASSERT_NE(b, DEFAULT_SWAPID);
        ASSERT_NE(a, b);
        ASSERT_NE(d3d::get_back_buffer_rt(a), nullptr);
        ASSERT_NE(d3d::get_back_buffer_rt(b), nullptr);
        d3d::finish_render_commands();
        d3d::remove_swapchain(a);
        d3d::finish_render_commands();
        ASSERT_NE(d3d::get_back_buffer_rt(b), nullptr);
        d3d::remove_swapchain(b);
        d3d::finish_render_commands();
    }

    HWND remainingWindow = makeWindow();
    ASSERT_NE(remainingWindow, nullptr);
    const SWAPID remaining = d3d::create_swapchain(remainingWindow);
    ASSERT_NE(remaining, DEFAULT_SWAPID);
    ASSERT_NE(d3d::get_back_buffer_rt(remaining), nullptr);
    d3d::finish_render_commands();
    // TearDown releases the driver while this additional window still exists.
}
int main(int argc, char** argv)
{
    nau::diag::setLogger(nau::diag::createLogger());
    nau::setDefaultServiceProvider(nau::createServiceProvider());
    cpujobs::init();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
