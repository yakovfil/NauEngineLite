// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include <emscripten.h>

#include <atomic>
#include <cstdlib>

#include "nau/app/application_services.h"
#include "nau/app/browser_runtime.h"
#include "nau/app/main_loop/game_system.h"
#include "nau/module/module_manager.h"
#include "nau/runtime/async_disposable.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"

using namespace nau;
namespace
{
    std::atomic<int32_t> releaseGate{0};
    int scenario = 0, updates = 0, disposed = 0, destroyed = 0, initialized = 0;
    int workCallbacks = 0;

    async::Task<> queueNoise()
    {
        for (int i = 0; i < 60; ++i)
        {
            co_await std::chrono::milliseconds(10);
            ++workCallbacks;
        }
    }

    async::Task<> waitGate()
    {
        while (!releaseGate.load())
            co_await std::chrono::milliseconds(10);
    }

    class FixtureService final : public IServiceInitialization,
                                 public IServiceShutdown,
                                 public IGamePreUpdate,
                                 public IAsyncDisposable
    {
        NAU_RTTI_CLASS(FixtureService, IServiceInitialization, IServiceShutdown, IGamePreUpdate, IAsyncDisposable)
    public:
        ~FixtureService()
        {
            ++destroyed;
        }
        async::Task<> preInitService() override
        {
            if (scenario == 3 || scenario == 12)
                co_await waitGate();
        }
        async::Task<> initService() override
        {
            ++initialized;
            if (scenario == 0 || scenario == 11)
                m_noise = queueNoise();
            if (scenario == 2)
                co_await std::chrono::milliseconds(50);
            if (scenario == 1 || scenario == 2)
                co_await Result<>{NauMakeError("Injected startup failure")};
        }
        async::Task<> shutdownService() override
        {
            if (m_noise)
                co_await m_noise;
            if (scenario == 5)
                co_await Result<>{NauMakeError("Injected shutdown failure")};
        }
        async::Task<> disposeAsync() override
        {
            ++disposed;
            if (scenario == 4 || scenario == 9)
                co_await waitGate();
        }
        void gamePreUpdate(std::chrono::milliseconds dt) override
        {
            ++updates;
            // clang-format off
            MAIN_THREAD_EM_ASM({ Module.measurements.push({dt:$0, time:performance.now()}); }, dt.count());
            // clang-format on
            if (scenario == 10 && updates == 3)
                std::abort();
        }

    private:
        async::Task<> m_noise;
    };

    class BatchFailure final : public IServiceInitialization
    {
        NAU_RTTI_CLASS(BatchFailure, IServiceInitialization)
        async::Task<> preInitService() override
        {
            co_await std::chrono::milliseconds(50);
            co_await Result<>{NauMakeError("Injected independent failure while another task is pending")};
        }
    };

    class Delegate final : public ApplicationInitDelegate
    {
        Result<> configureApplication() override
        {
            if (scenario == 6)
                return NauMakeError("Injected configuration failure");
            return {};
        }
        Result<> initializeApplication() override
        {
            NauCheckResult(loadModulesList(NAU_MODULES_LIST))
            getServiceProvider().addService(createPlatformWindowService());
            getServiceProvider().addService(eastl::make_unique<FixtureService>());
            if (scenario == 12)
                getServiceProvider().addService(eastl::make_unique<BatchFailure>());
            if (scenario == 7)
                return NauMakeError("Injected setup failure");
            return {};
        }
    };
}  // namespace

int main()
{
    scenario = MAIN_THREAD_EM_ASM_INT({ return Module.scenarioId; });
    // clang-format off
    MAIN_THREAD_EM_ASM({ Module.releaseWork = () => Atomics.store(HEAP32, $0 >> 2, 1); }, &releaseGate);
    // clang-format on
    Delegate delegate;
    const int code = runBrowserApplication(delegate, {.foregroundProgressTimeoutMs = scenario == 9 ? 250 : 10000});
    // clang-format off
    MAIN_THREAD_EM_ASM({ Module.fixtureStats = ({updates:$0, disposed:$1, destroyed:$2, initialized:$3, workCallbacks:$4}); }, updates, disposed, destroyed, initialized, workCallbacks);
    // clang-format on
    return scenario == 11 ? 2 : code;
}
