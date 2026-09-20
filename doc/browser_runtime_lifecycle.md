# Browser application lifecycle

`runBrowserApplication()` owns the real shared application on the Emscripten
application pthread. It links `NauFrameworkMinimal`, the production static
`PlatformApp` module and the minimal kernel. The independent acceptance target
remains a lifecycle fixture. The [actual sample](minimal_app_web.md) now adapts
`../NauSamples/minimalApp/app_sample_main_min.cpp`; runtime packaging remains separate.

## Build and test on Windows

Initialize the recorded bundled sources. Install CMake 3.24+, Ninja, external
emsdk **6.0.9**, Chrome and Python with `playwright==1.63.0`. The tested versions
are listed in the parent repository's `prerequisites.md`. Use a Python interpreter
with Playwright for the runner; the SDK separately uses its bundled Python.

From `NauEngineLite` in PowerShell:

```powershell
$runnerPython = (Get-Command python).Source
& $runnerPython -m pip install playwright==1.63.0
. "$HOME/emsdk/emsdk_env.ps1"
foreach ($configuration in @('Debug', 'Release')) {
    $runtimeBuild = "build/browser-runtime-$($configuration.ToLower())"
    cmake -S cmake/probes/browser_runtime -B $runtimeBuild -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$env:EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" "-DCMAKE_BUILD_TYPE=$configuration" "-DPython3_EXECUTABLE=$runnerPython"
    if ($LASTEXITCODE -ne 0) { throw 'Browser runtime configuration failed' }
    cmake --build $runtimeBuild --parallel 8
    if ($LASTEXITCODE -ne 0) { throw 'Browser runtime build failed' }
    ctest --test-dir $runtimeBuild --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Browser runtime tests failed' }
}
```

The target is `NauBrowserRuntimeTests`; its `.js` and `.wasm` outputs are in
`build/browser-runtime-<configuration>/bin`. CTest starts isolated Chrome profiles
and loopback servers with COOP `same-origin` and COEP `require-corp`. Real tab
visibility uses CDP with Playwright focus emulation disabled (`no_defaults=True`).
No renderer, GPU context, vcpkg, Qt, USD or desktop generator packages are needed.

Run a single case or select another Chrome installation:

```powershell
& $runnerPython cmake/probes/browser_runtime/run.py --build build/browser-runtime-debug --scenario success --chrome 'C:/Program Files/Google/Chrome/Application/chrome.exe'
```

Thirteen CTest cases cover success, immediate/deferred startup errors, Stop during
startup, pending cleanup, shutdown errors, configuration/setup failures, host
loss, stalled progress, an independent failure while another task is pending,
fatal abort and deliberate fixture failure. The last two run isolated negative
fixtures: their runner exits **1**, and a wrapper verifies the expected failure
instead of treating an infrastructure error as success. Direct `--scenario abort`
and `--scenario fixture-failure` commands intentionally fail.

## Application integration

Include `nau/app/browser_runtime.h`. From the worker entry point, pass an
`ApplicationInitDelegate` to `nau::runBrowserApplication(delegate)`. In the
delegate's `initializeApplication()`, load `NAU_MODULES_LIST` and register
`createPlatformWindowService()`, together with application services. Link the
production module using `nau_target_link_modules(target PlatformApp)` and the
shared threaded Emscripten options (`PROXY_TO_PTHREAD`, static libraries).
The driver returns 0 after orderly cleanup or 1 after failure. It accepts only
one invocation per page and rejects a call on the browser UI thread.

Provide a connected `#nau-window` HTMLElement before loading the generated
JavaScript, or select it with `Module.nauWindowSelector`. See
[the platform contract](browser_platform_services.md). Page controls can use:

```javascript
stopButton.onclick = () => Module.nauRuntime?.stop();
// Read the latest immutable snapshot without calling into the worker.
const snapshot = Module.nauRuntime?.status;
// snapshot: state, phase, message, completedSteps, sequence, cleanupComplete
```

States are Starting, Running, Stopping, Stopped and Failed. `completedSteps`
counts completed game updates only. `sequence` increases for every accepted
snapshot; a late snapshot is rejected. Failures retain their original phase and
diagnostic while collecting later cleanup errors. Failed remains terminal even
when `cleanupComplete` subsequently becomes true. `start()` rejects with reload
guidance; retry after failure or a normal stop requires a page reload.

The UI's Stop operation only updates page-lifetime atomic control words and wakes
the worker; it never waits for cleanup or keeps an Application pointer. Repeated
Stop, including after completion, is safe. Final status is delivered synchronously
to the UI from the worker after application destruction; the UI never waits on
the worker. Visibility/watchdog listeners detach at completion. A bounded history
of the latest 1024 worker-delivered snapshots supports diagnostics.

## Timing and cleanup

Game updates use monotonic 100 ms deadlines, independent of executor and Stop
wakes. Missed deadlines are skipped. Browser elapsed time is clamped to 0–100 ms;
a detected gap above 250 ms discards the old elapsed baseline. Hide/show resets
the baseline. Hidden execution is best-effort, with no frequency guarantee.
The normal native `Application::step()` timing policy is unchanged.

`BrowserRuntimeOptions::foregroundProgressTimeoutMs` defaults to 10000. It tracks
lifecycle phase changes, completed service tasks and game updates, rather than
counting an idle polling loop as progress. Hidden time resets the deadline.
Expiry reports Failed with incomplete cleanup, retains live owners and continues
cooperative cleanup when pending work is released. The UI watchdog separately
reports a worker that stops delivering snapshots. It cannot recover non-returning
callbacks. An observed `Module.onAbort` reports Failed with incomplete cleanup
and invokes the previous host hook, without calling back into aborted Wasm.

Stop during startup latches intent and prevents later phases and Running. Started
tasks settle before release. Shutdown hooks run for initialized services;
disposal covers instantiated objects, with runtime-registry ownership preventing
duplicate disposal. Cleanup continues executor work without starting game updates.
The native scene cleanup path additionally pumps update/post-update continuations
for work accepted before Stop, retaining scene ownership until completion.

## Checked shared APIs

`createApplicationChecked(delegate)` returns `Result<eastl::unique_ptr<Application>>`
and unwinds partial creation on error. The legacy factory remains available and
returns nullptr with a diagnostic on creation failure. Successful legacy
`startup()`/`step()`/`stop()` callers retain their signatures.

For another owner-thread driver, query `ApplicationLifecycle` through Nau RTTI.
`beginStartup()` starts initialization, `pollLifecycle()` advances startup and
cleanup without game updates, `getPhase()` and `getLifecycleError()` expose the
result, and `stepWithElapsedTime()` performs an eligible update. Its caller owns
the timing policy. `getLifecycleProgress()` is an opaque service-work change
token, not a monotonic task count. Only `Application::stop()` is cross-thread;
other lifecycle calls belong to the application's owning thread.

The parent change's `validation.md` records browser/native results and the
remaining sample, preset, packaging and relocation boundary.
