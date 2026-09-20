# Browser platform services

The production PlatformApp module now supplies a non-rendering browser window
manager. SDK 6.0.9 and Chrome 153.0.8010.52 pass the focused platform tests in
Debug and Release on Windows. The [browser lifecycle driver](browser_runtime_lifecycle.md)
now provides application cadence, checked startup and Stop controls. The [actual sample](minimal_app_web.md) now consumes that driver;
relocatable application delivery remains separate work.

## Build and test

From `NauEngineLite`, with the recorded bundled sources initialized:

```powershell
python -m pip install playwright==1.63.0
. "$HOME/emsdk/emsdk_env.ps1"
foreach ($configuration in @('Debug', 'Release')) {
    $platformBuild = "build/browser-platform-$($configuration.ToLower())"
    cmake -S cmake/probes/browser_platform -B $platformBuild -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$env:EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" "-DCMAKE_BUILD_TYPE=$configuration" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    if ($LASTEXITCODE -ne 0) { throw 'Browser platform configuration failed' }
    cmake --build $platformBuild --parallel 8
    if ($LASTEXITCODE -ne 0) { throw 'Browser platform build failed' }
    ctest --test-dir $platformBuild --output-on-failure
    if ($LASTEXITCODE -ne 0) { throw 'Browser platform tests failed' }
}
```

Use the same Python interpreter for pip and CMake's Python3 discovery. Validation
used system Python 3.14.2; the SDK compiler separately uses its bundled Python.
If needed, set `-DPython3_EXECUTABLE=<absolute-python-path>` when configuring.
Prerequisites are CMake 3.24+ (tested 4.2.1), Ninja 1.13.2, activated emsdk 6.0.9,
the recorded minimal source dependencies and googletest, Python/Playwright and
Chrome. No vcpkg, renderer, Qt, USD or graphics context is used by this fixture.

CTest runs eight cases per configuration: valid host, missing host, ambiguous
selector, non-HTMLElement host, injected callback-installation failure, missing
backend, host removal and device pixel ratio 2. Tests link the real NauKernel,
NauFrameworkMinimal and PlatformApp with generated production static registration.

The runner starts a loopback server with COOP/COEP headers and Chrome with an
isolated temporary profile, captures JSON results/diagnostics and enforces test
timeouts. It connects with Playwright `no_defaults=True` because Playwright's
default focus emulation prevents real document visibility transitions. Chrome
itself backgrounds and restores the fixture tab; visibility is not mocked.
All helper processes are closed after each run. The fixture's HTML is copied to
the ignored build/bin directory and is not an application delivery package.

For a single case or a nonstandard Chrome installation:

```powershell
python cmake/probes/browser_platform/run.py --build build/browser-platform-debug --scenario valid
python cmake/probes/browser_platform/run.py --build build/browser-platform-debug --dpr 2 --chrome 'C:/Program Files/Google/Chrome/Application/chrome.exe'
python cmake/probes/browser_platform/run.py --build build/browser-platform-debug --scenario fixture-failure
```

The last command intentionally fails with exit code 1 and a GoogleTest failure;
it verifies that the runner does not turn a failing fixture into success.

## Host contract

The page supplies exactly one connected HTMLElement. Set `Module.nauWindowSelector`
before initialization to select it; the default is `#nau-window`. The module does
not create or remove the element, and extra createWindow calls return empty with a
diagnostic. DOM access runs on the UI thread through short synchronous Emscripten
proxies. The application and platform service run on workers. Do not block the
browser UI waiting for worker work or call worker-owned disposal from the UI.

- Sizes are measured integer CSS pixels: outer layout border box versus client
  area. setSize compensates for padding/borders, with page CSS constraints still
  applying. Device pixel ratio does not scale the reported sizes.
- setVisible changes CSS visibility and preserves layout space. isVisible checks
  computed visibility and ancestor display policy; it does not detect occlusion.
  Initial geometry and visibility come from the page.
- IBrowserWindowStatus exposes document visibility separately and reports
  Initializing, Ready, Closing, Closed or Failed. Host disconnection produces a
  failure diagnostic and does not bind a replacement element.
- setName changes the host's accessible label. It does not change document.title.
  Desktop positioning is unsupported: setPosition diagnoses it and getPosition
  returns the documented unavailable value `{0, 0}`.

IPlatformWindowInitialization is an optional additive readiness contract.
PlatformWindowService publishes the browser manager only after successful binding;
failed initialization settles the service task, cleans partial resources and joins
its worker. The existing Windows backend keeps its prior binding path. Public
ApplicationImpl now preserves service startup results through the checked
[application lifecycle](browser_runtime_lifecycle.md), including partial cleanup.

## Shutdown

Closing rejects new window operations, prevents further queue admission, accounts
for in-flight DOM calls, detaches callbacks, drains accepted executor invocations
and wakes the service pump. Worker completion precedes asynchronous service
disposal completion. Repeated disposal and releasing retained references are safe.

The kernel's generic Executor::Invocation has no cancellation/rejection operation.
Accepted callbacks therefore run to completion. A submission to the closing or
closed executor is diagnosed and completed on its submitting thread rather than
silently discarding a coroutine continuation. Such late submissions have no
platform-thread affinity; window operations still reject access after closing.
This fallback adds no kernel API and does not resurrect the service worker.

## Validation limits

Native minimal lifecycle tests pass in Debug/Release. The approved kernel
source-selection fix removes Emscripten sources from the Windows graph; the full
`test_scene` dependency build, including NauFramework and PlatformApp, and 12
affected scene tests now pass in both configurations. Full desktop applications
are outside this boundary. Wasm core tests pass under SDK Node 24.19.0.

Browser application presets now build the [actual sample](minimal_app_web.md).
Runtime staging and delivery relocation remain pending. This platform fixture proves production module and
service behavior, not application stepping or package relocation.
