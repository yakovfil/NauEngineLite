// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>

#include "nau/app/application.h"
#include "nau/app/browser_runtime.h"
#include "nau/app/run_application.h"
#include "nau/async/task.h"
#include "nau/module/module_manager.h"
#include "nau/service/service.h"
#include "nau/service/service_provider.h"
#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

using namespace nau;
namespace
{
    std::string scenario;
    int pre = 0, post = 0, legacy = 0, initialized = 0, startup = 0;
    int servicesDestroyed = 0, delegatesDestroyed = 0;
    bool ordered = true;
    int runtimePolls = 0, compatibilityPolls = 0, cleanupPolls = 0;
    bool runtimeFailure = false, startupFinished = false, runtimeCleanupFinished = false;
    std::optional<async::TaskSource<>> startupGate, cleanupGate;
    bool runtimeScenario()
    {
        return scenario.starts_with("runtime");
    }
    std::chrono::milliseconds elapsed{0};
    class Resource final : public IServiceInitialization,
                           public IServiceShutdown
    {
        NAU_RTTI_CLASS(Resource, IServiceInitialization, IServiceShutdown)
    public:
        ~Resource()
        {
            ++servicesDestroyed;
        }
        async::Task<> initService() override
        {
            co_return;
        }
        async::Task<> shutdownService() override
        {
            co_await std::chrono::milliseconds(10);
            if (scenario == "shutdownerror")
                co_await Result<>{NauMakeError("host cleanup failure")};
        }
    };
    class RuntimeResource final : public IServiceInitialization,
                                  public IServiceShutdown,
                                  public IApplicationRuntimeService
    {
        NAU_RTTI_CLASS(RuntimeResource, IServiceInitialization, IServiceShutdown, IApplicationRuntimeService)
    public:
        ~RuntimeResource()
        {
            ++servicesDestroyed;
        }
        Result<> pollApplicationRuntime() override
        {
            ++runtimePolls;
            ordered = ordered && getApplication().isMainThread();
            if (startupGate && !startupFinished)
            {
                ordered = ordered && pre == 0 && post == 0;
                if (runtimePolls >= 3)
                {
                    auto gate = std::move(*startupGate);
                    startupGate.reset();
                    gate.resolve();
                    if (scenario == "runtime-startup-error" && !runtimeFailure)
                    {
                        runtimeFailure = true;
                        return NauMakeError("host runtime startup polling failure");
                    }
                }
            }
            if (cleanupGate)
            {
                ++cleanupPolls;
                if (cleanupPolls >= 3)
                {
                    auto gate = std::move(*cleanupGate);
                    cleanupGate.reset();
                    gate.resolve();
                }
                if (scenario == "runtime-cleanup-error" && !runtimeFailure)
                {
                    runtimeFailure = true;
                    return NauMakeError("host runtime cleanup polling failure");
                }
            }
            if (scenario == "runtime-error" && post == 2 && !runtimeFailure)
            {
                runtimeFailure = true;
                return NauMakeError("host runtime polling failure");
            }
            return {};
        }
        async::Task<> shutdownService() override
        {
            cleanupGate.emplace();
            co_await cleanupGate->getTask();
            runtimeCleanupFinished = true;
        }
    };
    class CompatibilityResource final : public IServiceInitialization,
                                        public IServiceShutdown,
                                        public IBrowserRuntimeService
    {
        NAU_RTTI_CLASS(CompatibilityResource, IServiceInitialization, IServiceShutdown, IBrowserRuntimeService)
    public:
        ~CompatibilityResource()
        {
            ++servicesDestroyed;
        }
        Result<> pollBrowserRuntime() override
        {
            ++compatibilityPolls;
            ordered = ordered && getApplication().isMainThread();
            return {};
        }
        async::Task<> shutdownService() override
        {
            co_return;
        }
    };
    class Delegate : public ApplicationDelegate
    {
    public:
        ~Delegate()
        {
            ++delegatesDestroyed;
            const bool valid = ordered && delegatesDestroyed == 1 && !applicationExists() && !hasServiceProvider() && (scenario == "configfail" || servicesDestroyed == (runtimeScenario() ? 3 : 1)) &&
                               (!runtimeScenario() || (runtimePolls > 0 && runtimePolls == compatibilityPolls && cleanupPolls >= 3 && runtimeCleanupFinished && startupFinished));
            char json[512];
            std::snprintf(json, sizeof(json), "{\"valid\":%s,\"pre\":%d,\"post\":%d,\"legacy\":%d,\"initialized\":%d,\"startup\":%d,\"servicesDestroyed\":%d,\"delegatesDestroyed\":%d}",
                          valid ? "true" : "false", pre, post, legacy, initialized, startup, servicesDestroyed, delegatesDestroyed);
            std::printf("HOST_RESULT %s\n", json);
#ifdef __EMSCRIPTEN__
            MAIN_THREAD_EM_ASM({ Module.fixtureStats = JSON.parse(UTF8ToString($0)); }, json);
#endif
        }
        Result<> configureApplication() override
        {
            return scenario == "configfail" ? Result<>{NauMakeError("host configuration failure")} : Result<>{};
        }
        std::string getModulesListString() const override
        {
            return NAU_MODULES_LIST;
        }
        Result<> initializeServices() override
        {
            getServiceProvider().addService(std::make_unique<Resource>());
            if (runtimeScenario())
            {
                getServiceProvider().addService(std::make_unique<RuntimeResource>());
                getServiceProvider().addService(std::make_unique<CompatibilityResource>());
            }
            return scenario == "setupfail" ? Result<>{NauMakeError("host setup failure")} : Result<>{};
        }
        void onApplicationInitialized() override
        {
            ++initialized;
        }
        async::Task<> startupApplication() override
        {
            ++startup;
            if (runtimeScenario())
            {
                startupGate.emplace();
                co_await startupGate->getTask();
                startupFinished = true;
            }
            if (scenario == "immediate")
                co_await Result<>{NauMakeError("host startup failure")};
            if (scenario == "deferred")
            {
                co_await std::chrono::milliseconds(30);
                co_await Result<>{NauMakeError("host startup failure")};
            }
            if (scenario == "stopstartup")
            {
                co_await std::chrono::milliseconds(10);
                getApplication().stop();
                getApplication().stop();
                co_await std::chrono::milliseconds(60);
            }
            if (scenario == "pending" || scenario == "stall")
                co_await std::chrono::milliseconds(scenario == "stall" ? 350 : 100);
        }
        void onApplicationPreUpdate(std::chrono::milliseconds dt) override
        {
            ordered = ordered && pre == post;
            elapsed = dt;
            ++pre;
            if (scenario == "stoppre")
                getApplication().stop();
        }
        void onApplicationStep(std::chrono::milliseconds dt) override
        {
            ++legacy;
            finishStep(dt);
        }

    protected:
        void finishStep(std::chrono::milliseconds dt)
        {
            ordered = ordered && pre == post + 1 && dt == elapsed;
            ++post;
            if (post == 4 && scenario != "visibility")
                getApplication().stop();
        }
    };
    class PhaseDelegate final : public Delegate
    {
        void onApplicationPostUpdate(std::chrono::milliseconds dt) override
        {
            finishStep(dt);
        }
    };
}  // namespace
int main(int argc, char** argv)
{
    scenario = argc > 1 ? argv[1] : "success";
    auto options = ApplicationHostOptions{};
    if (scenario != "fixed")
        options.scheduling = ApplicationScheduling::Presentation;
    if (scenario == "stall")
        options.foregroundProgressTimeoutMs = 100;
    return runApplication(scenario == "legacy" ? ApplicationDelegate::Ptr{std::make_unique<Delegate>()} : ApplicationDelegate::Ptr{std::make_unique<PhaseDelegate>()}, options);
}
