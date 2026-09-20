// Copyright 2026 N-GINN LLC. All rights reserved.
// Use of this source code is governed by a BSD-3 Clause license that can be found in the LICENSE file.

#include "nau/app/browser_runtime.h"

#include <emscripten.h>
#include <emscripten/threading.h>

#include <atomic>
#include <chrono>

#include "nau/app/application_lifecycle.h"
#include "nau/app/application_services.h"
#include "nau/app/browser_runtime_timing.h"
#include "nau/app/platform_window_status.h"
#include "nau/async/work_queue.h"
#include "nau/service/service_provider.h"

namespace nau
{
    namespace
    {
        // These control words live until page destruction; UI callbacks never
        // retain an Application pointer, executor or coroutine context.
        std::atomic<int32_t> stopRequested{0};
        std::atomic<int32_t> wakeSequence{0};
        std::atomic<int32_t> watchdogFailed{0};
        std::atomic<int32_t> documentVisible{1};
        std::atomic<bool> started{false};
        static_assert(sizeof(std::atomic<int32_t>) == 4 && std::atomic<int32_t>::is_always_lock_free);

        void wakeWorker()
        {
            ++wakeSequence;
            emscripten_futex_wake(&wakeSequence, 1);
        }

        int64_t nowMs()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        const char* phaseName(ApplicationPhase phase)
        {
            switch (phase)
            {
                case ApplicationPhase::Created:
                    return "Creation";
                case ApplicationPhase::PreInitializing:
                    return "Pre-initialization";
                case ApplicationPhase::Initializing:
                    return "Initialization";
                case ApplicationPhase::Running:
                    return "Running";
                case ApplicationPhase::Stopping:
                    return "Cleanup";
                case ApplicationPhase::Stopped:
                    return "Complete";
            }
            return "Unknown";
        }

        // clang-format off
        void installBridge(int timeout)
        {
            MAIN_THREAD_EM_ASM({ (() => {
                const stop = $0 >> 2, wake = $1 >> 2, failed = $2 >> 2;
                const bridge = {
                    status: null, history: [], lastWorkerSequence: 0, sequence: 0,
                    failed: false, aborted: false, lastHeartbeat: performance.now(),
                    stop() { Atomics.store(HEAP32, stop, 1); Atomics.add(HEAP32, wake, 1); Atomics.notify(HEAP32, wake); },
                    start() { return {accepted:false, message:'Reload the page to retry'}; },
                    deliver(snapshot) {
                        if (this.aborted || snapshot.sequence <= this.lastWorkerSequence) return false;
                        this.lastWorkerSequence = snapshot.sequence;
                        this.lastHeartbeat = performance.now();
                        if (this.failed) snapshot = {...snapshot, state:'Failed', message:snapshot.message || this.status.message};
                        if (snapshot.state === 'Failed') this.failed = true;
                        this.status = Object.freeze({...snapshot, sequence:++this.sequence});
                        this.history.push(this.status);
                        if (this.history.length > 1024) this.history.shift();
                        if (this.status.cleanupComplete) this.detach();
                        return true;
                    },
                    detach() { clearInterval(this.watchdog); document.removeEventListener('visibilitychange', this.visibility); }
                };
                Module.nauRuntime = bridge;
                bridge.visibility = () => {
                    bridge.lastHeartbeat = performance.now();
                    Atomics.store(HEAP32, $4 >> 2, document.hidden ? 0 : 1);
                    Atomics.add(HEAP32, wake, 1); Atomics.notify(HEAP32, wake);
                };
                bridge.visibility();
                document.addEventListener('visibilitychange', bridge.visibility);
                bridge.watchdog = setInterval(() => {
                    if (document.hidden || !bridge.status || bridge.status.cleanupComplete || bridge.failed) return;
                    if (performance.now() - bridge.lastHeartbeat <= $3) return;
                    bridge.failed = true;
                    bridge.status = Object.freeze({...bridge.status, state:'Failed', cleanupComplete:false,
                        message:'Foreground worker progress deadline expired', sequence:++bridge.sequence});
                    bridge.history.push(bridge.status);
                    Atomics.store(HEAP32, failed, 1); bridge.stop();
                }, Math.min(100, $3));
                const previousAbort = Module.onAbort;
                Module.onAbort = reason => {
                    bridge.aborted = bridge.failed = true;
                    bridge.status = Object.freeze({...bridge.status, state:'Failed', cleanupComplete:false,
                        phase:'Fatal abort', message:String(reason), sequence:++bridge.sequence});
                    bridge.history.push(bridge.status); bridge.detach();
                    if (previousAbort) previousAbort(reason);
                };
            })(); }, &stopRequested, &wakeSequence, &watchdogFailed, timeout, &documentVisible);
        }

        void publish(const char* state, const char* phase, const eastl::string& message, int steps, int sequence, bool complete)
        {
            MAIN_THREAD_EM_ASM({
                Module.nauRuntime.deliver({state:UTF8ToString($0), phase:UTF8ToString($1),
                    message:UTF8ToString($2), completedSteps:$3, sequence:$4, cleanupComplete:!!$5,
                    applicationWorker:true, timestamp:performance.now()});
            }, state, phase, message.c_str(), steps, sequence, complete);
        }
// clang-format on
}

namespace
{
    struct BrowserLoop
    {
        eastl::unique_ptr<Application> app;
        ApplicationLifecycle& lifecycle;
        WorkQueue* queue;
        int timeout;
        bool cooperative;
        int sequence = 1, steps = 0;
        eastl::string failure;
        const char* failurePhase = nullptr;
        browser_detail::StepDeadline deadline;
        int64_t lastProgress = nowMs(), lastPublish = lastProgress;
        ApplicationPhase previousPhase;
        uint64_t previousProgress;
        bool wasVisible = true, alive = true;

        BrowserLoop(eastl::unique_ptr<Application> application, int timeoutMs, bool yield) :
            app(std::move(application)),
            lifecycle(app->as<ApplicationLifecycle&>()),
            queue(nullptr),
            timeout(timeoutMs),
            cooperative(yield)
        {
            if (stopRequested.load())
                app->stop();
            lifecycle.beginStartup();
            queue = app->getExecutor()->as<WorkQueue*>();
            queue->setWakeCallback(wakeWorker);
            deadline.reset(nowMs());
            previousPhase = lifecycle.getPhase();
            previousProgress = lifecycle.getLifecycleProgress();
        }
        void tick()
        {
            const auto wake = wakeSequence.load();
            const auto now = nowMs();
            bool visible = documentVisible.load() != 0;
            if (stopRequested.load())
                app->stop();
            for (auto* service : getServiceProvider().getAll<IBrowserRuntimeService>())
            {
                auto result = service->pollBrowserRuntime();
                if (!result && failure.empty())
                    failure = result.getError()->getDiagMessage();
            }
            if (auto* window = getServiceProvider().find<IBrowserWindowStatus>())
            {
                if (window->getBrowserState() == BrowserWindowState::Ready)
                    visible = visible && window->isDocumentVisible();
                if (window->getBrowserState() == BrowserWindowState::Failed && failure.empty())
                    failure = "Platform host lost";
            }
            if (visible != wasVisible)
            {
                deadline.reset(now);
                lastProgress = now;
                wasVisible = visible;
            }
            if (!visible)
                lastProgress = now;
            if (watchdogFailed.load() && failure.empty())
                failure = "Foreground worker progress deadline expired";
            if (auto error = lifecycle.getLifecycleError())
            {
                const auto diagnostic = error->getDiagMessage();
                if (failure.empty())
                    failure = diagnostic;
                else if (failure.find(diagnostic) == eastl::string::npos)
                    failure += eastl::string("\n") + diagnostic;
            }
            if (stopRequested.load() || !failure.empty())
                app->stop();
            if (!failure.empty() && !failurePhase)
                failurePhase = phaseName(lifecycle.getPhase());
            alive = lifecycle.pollLifecycle();
            const auto phase = lifecycle.getPhase();
            const auto progress = lifecycle.getLifecycleProgress();
            if (progress != previousProgress)
                lastProgress = now;
            previousProgress = progress;
            if (phase != previousPhase)
            {
                lastProgress = now;
                deadline.reset(now);
            }
            if (auto error = lifecycle.getLifecycleError())
            {
                const auto diagnostic = error->getDiagMessage();
                if (failure.empty())
                    failure = diagnostic;
                else if (failure.find(diagnostic) == eastl::string::npos)
                    failure += eastl::string("\n") + diagnostic;
            }
            if (visible && alive && now - lastProgress >= timeout && failure.empty())
            {
                failure = "Foreground lifecycle progress deadline expired";
                app->stop();
            }
            if (!failure.empty() && !failurePhase)
            {
                const bool wasStarting = previousPhase == ApplicationPhase::PreInitializing || previousPhase == ApplicationPhase::Initializing;
                const bool isStarting = phase == ApplicationPhase::PreInitializing || phase == ApplicationPhase::Initializing;
                failurePhase = phaseName(wasStarting && !isStarting ? previousPhase : phase);
            }
            bool stepped = false;
            if (alive && phase == ApplicationPhase::Running && failure.empty() && !stopRequested.load() && deadline.due(now))
            {
                stepped = lifecycle.stepWithElapsedTime(std::chrono::milliseconds(deadline.elapsedAndAdvance(now)));
                if (stepped)
                {
                    ++steps;
                    lastProgress = now;
                }
            }
            const char* state = !failure.empty() ? "Failed" : !alive                                                      ? "Stopped"
                                                          : (stopRequested.load() || phase == ApplicationPhase::Stopping) ? "Stopping"
                                                          : phase == ApplicationPhase::Running                            ? "Running"
                                                                                                                          : "Starting";
            if (!alive || stepped || phase != previousPhase || now - lastPublish >= 50 || !failure.empty())
            {
                // Publish final cleanup only after Application and RuntimeState destruction.
                if (!alive)
                {
                    queue->setWakeCallback(nullptr);
                    app.reset();
                }
                publish(state, failurePhase ? failurePhase : phaseName(phase), failure, steps, ++sequence, !alive);
                lastPublish = now;
            }
            previousPhase = phase;
            if (alive && !cooperative)
            {
                const int wait = phase == ApplicationPhase::Running && failure.empty() ? deadline.waitTime(nowMs()) : 5;
                emscripten_futex_wait(&wakeSequence, wake, wait);
            }
        }
    };
}

int runBrowserApplication(ApplicationInitDelegate& delegate, BrowserRuntimeOptions options)
{
    if (emscripten_is_main_browser_thread() || started.exchange(true))
        return 1;
    const int timeout = std::max(1, options.foregroundProgressTimeoutMs);
    installBridge(timeout);
    publish("Starting", "Creation", {}, 0, 1, false);
    auto creation = createApplicationChecked(delegate);
    if (!creation)
    {
        publish("Failed", "Creation", creation.getError()->getDiagMessage(), 0, 2, true);
        return 1;
    }
    if (options.cooperativePresentation)
    {
        // A heap-owned loop survives returning to the worker's JavaScript event
        // loop. Chrome presents OffscreenCanvas frames there. No Asyncify and no
        // additional game steps: tick retains the shared deadline policy.
        auto* loop = new BrowserLoop(std::move(*creation), timeout, true);
        emscripten_set_main_loop_arg([](void* argument)
        {
            auto* loop = static_cast<BrowserLoop*>(argument);
            loop->tick();
            if (!loop->alive)
            {
                const int code = loop->failure.empty() ? 0 : 1;
                emscripten_cancel_main_loop();
                delete loop;
                emscripten_force_exit(code);
            }
        }, loop, 200, true);
        return 1;  // simulate_infinite_loop unwinds to the worker scheduler.
    }
    BrowserLoop loop(std::move(*creation), timeout, false);
    while (loop.alive)
        loop.tick();
    return loop.failure.empty() ? 0 : 1;
}
}
