// Copyright 2024 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "render_system.h"

#include <imgui.h>

#include <span>
#include <string>
#include <vector>

#include "graphics_assets/shader_asset.h"
#include "nau/app/core_window_manager.h"
#include "nau/app/platform_window.h"
#include "nau/app/window_manager.h"
#include "nau/diag/logging.h"
#include "nau/gui/dag_imgui.h"
#include "nau/gui/imguiInput.h"
#include "nau/osApiWrappers/dag_cpuJobs.h"
#include "nau/service/service_provider.h"
#include "nau/utils/performance_profiling.h"
#include "render/daBfg/bfg.h"
#define USE_SHADER_CACHE 1

class Driver3dInitCB : public Driver3dInitCallback
{
public:
    void verifyResolutionSettings(
        int& ref_scr_wdt,
        int& ref_scr_hgt,
        int base_scr_wdt,
        int base_scr_hgt,
        bool window_mode) const override
    {
        if ((ref_scr_wdt > base_scr_wdt || ref_scr_hgt > base_scr_hgt) && window_mode)
        {
            // Nothing to do.
        }
        else
        {
            allowResolutionOverlarge = -1;
        }

        if (allowResolutionOverlarge == 1)
        {
            ref_scr_wdt = base_scr_wdt;
            ref_scr_hgt = base_scr_hgt;
        }
    }

    int validateDesc(Driver3dDesc&) const override
    {
        return 1;
    }

    int compareDesc(Driver3dDesc&, Driver3dDesc&) const override
    {
        return 0;
    }

    bool desiredStereoRender() const override
    {
        return false;
    }

    int64_t desiredAdapter() const override
    {
        return 0;
    }

    RenderSize desiredRendererSize() const override
    {
        return {};
    }

    const char* desiredRendererDeviceExtensions() const override
    {
        return nullptr;
    }

    const char* desiredRendererInstanceExtensions() const override
    {
        return nullptr;
    }

    VersionRange desiredRendererVersionRange() const override
    {
        return {0, 0};
    }

    mutable int allowResolutionOverlarge = -1;
};

static Driver3dInitCB cb;

namespace nau
{
    RenderSystem::RenderSystem() = default;

    RenderSystem::~RenderSystem() = default;

    RenderScene::Ptr RenderSystem::createScene(const std::string& sceneName)
    {
        NAU_ASSERT(m_scenes.count(sceneName) == 0);

        m_scenes[sceneName] = nau::rtti::createInstance<RenderScene>();
        return m_scenes[sceneName];
    }


    async::Task<> RenderSystem::preInitService()
    {
        bool isDriverInited = d3d::init_driver();
        NAU_ASSERT(isDriverInited);
        unsigned memSizeKb = d3d::get_dedicated_gpu_memory_size_kb();

        const char* gameName = "render test";
        uint32_t gameVersion = 1;

        d3d::driver_command(DRV3D_COMMAND_SET_APP_INFO, (void*)gameName, (void*)&gameVersion, nullptr);

        d3d::update_window_mode();

        d3d::driver_command(DRV3D_COMMAND_ENABLE_MT, NULL, NULL, NULL);
        ::enable_tex_mgr_mt(true, 1024);

        cpujobs::init();


        IFrameAllocator::setFrameAllocator(&m_frameAllocator);


        IWindowManager& wndManager = getServiceProvider().get<IWindowManager>();
        HWND hwnd = wndManager.getActiveWindow().as<IWindowsWindow*>()->getWindowHandle();

        void* hwndv = hwnd;
        HINSTANCE hinst = GetModuleHandle(nullptr);
        const char* title = "render test";
        int ncmd = 5;
        const char* wcName = "wcName";

        main_wnd_f* wndProc = nullptr;

        d3d::init_video(hinst, wndProc, wcName, ncmd, hwndv, hwndv, nullptr, title, &cb);

        m_rendState.cull = CULL_CCW;
        m_drvRendStateId = d3d::create_render_state(m_rendState);

        int posx, posy, width, height;
        float minz, maxz;
        d3d::getview(posx, posy, width, height, minz, maxz);
        d3d::setview(posx, posy, width, height, 0.0f, 1.0f);


        imgui_switch_state();
        imgui_update();  // invoke init on demand
        imgui_endframe();

        nau::hal::init_main_thread_id();

        dabfg::startup();

        m_nodeHandles.clear();

        m_isInitialized = true;

        co_return;
    }


    async::Task<> RenderSystem::initService()
    {
        return async::makeResolvedTask();
    }


    async::Task<bool> RenderSystem::renderFrame()
    {
        NAU_FATAL(m_isInitialized, "Service GraphicsImpl is not initialized!");
        using namespace nau::async;
        if (m_isDisposed)
        {
            if (!m_renderStopedSignal.isReady())
            {
                stopGraphics();
                m_renderStopedSignal.resolve();
            }

            co_return false;
        }

        IFrameAllocator* frameAllocator = IFrameAllocator::getFrameAllocator();
        NAU_ASSERT(frameAllocator, "Frame allocator is not initialized");
        bool hasPrepared = frameAllocator->prepareFrame();
        NAU_ASSERT(hasPrepared);
        
        co_await executeRenderJobs();

        renderMainScene();

        co_return true;
    }

    async::Task<bool> RenderSystem::update([[maybe_unused]] std::chrono::milliseconds dt)
    {
        co_return renderFrame();
    }


    void RenderSystem::renderMainScene()
    {
// TODO Tracy        NAU_CPU_SCOPED_TAG(nau::PerfTag::Render);

    }


    void RenderSystem::stopGraphics()
    {
        imgui_shutdown();

        m_nodeHandles.clear();
        dabfg::shutdown();
        d3d::release_driver();
    }


    async::Task<> RenderSystem::shutdownService()
    {
        if (const bool alreadyDisposed = m_isDisposed.exchange(true); !alreadyDisposed)
        {
            NAU_LOG_DEBUG("Graphics shutdown started");
            co_await m_renderStopedSignal.getTask();
            NAU_LOG_DEBUG("Graphics shutdown completed");
        }
    }

    async::Task<> RenderSystem::activateComponentsAsync(Uid worldUid, std::span<const scene::Component*> components, async::Task<> barrier)
    {
        return async::makeResolvedTask();
    }


    void RenderSystem::syncSceneState()
    {
        imgui_copy_render_data();
    }


    async::Executor::Ptr RenderSystem::getPreRenderExecutor()
    {
        return m_preRenderWorkQueue;
    }


    void RenderSystem::addPreRenderJob(AsyncAction action)
    {
        NAU_FATAL(action);
        lock_(m_preRenderJobsMutex);
        m_preRenderJobs.emplace_back(std::move(action));
    }


    async::Task<> RenderSystem::executeRenderJobs()
    {
        using namespace nau::async;

        m_preRenderWorkQueue->poll();

        decltype(m_preRenderJobs) jobs;
        {
            lock_(m_preRenderJobsMutex);
            if (m_preRenderJobs.empty())
            {
                co_return;
            }

            jobs = std::move(m_preRenderJobs);
            m_preRenderJobs.clear();
        }

        std::vector<Task<>> tasks;
        tasks.reserve(jobs.size());

        for (auto& work : jobs)
        {
            if (Task<> task = work(); !task.isReady())
            {
                tasks.emplace_back(std::move(task));
            }
        }

        co_await whenAll(tasks, Expiration::never());
    }

}  // namespace nau
