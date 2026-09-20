# Minimal browser sample

`MinimalAppSample` now builds from `samples/minimalApp/app_sample_main_min.cpp`
through the production `web-minimal-debug` and `web-minimal-release` presets.
The sample uses the real shared application, minimal kernel and statically
registered browser PlatformApp. It runs on an application worker without a
renderer, assets or GPU context. Native minimal sample builds remain supported.

## Build on Windows

Use the recorded source dependencies, CMake 3.24+, Ninja and external emsdk
**6.0.9**. See [SDK setup](emscripten_build.md) and the parent repository's
`prerequisites.md`. From `NauEngineLite` in PowerShell:

```powershell
. "$HOME/emsdk/emsdk_env.ps1"
./cmake/check_emscripten.ps1
cmake --preset web-minimal-debug
cmake --build --preset web-minimal-debug --parallel 8
cmake --preset web-minimal-release
cmake --build --preset web-minimal-release --parallel 8
```

Both builds produce `bin/MinimalAppSample.js` and `bin/MinimalAppSample.wasm`
under their respective `build/web-minimal-*` directory. SDK 6.0.9 embeds worker
support in the loader for these builds; no separate worker script was emitted.
Each configuration uses its own `lib` directory. No runtime package is staged by
this change, and proposed `package` directories are not current outputs.

The presets use static pthread libraries, worker entry-point execution, a 1 MiB
stack and a prewarmed pool of 12 workers. Runtime exit is enabled and blocking
on the browser UI thread is disallowed. Debug uses diagnostics; Release uses
optimization. Browser builds reject the desktop profile and shared-library mode.
They do not require vcpkg, Qt, USD, DirectX, renderer tools, desktop generator
Python packages or the test runner's Python/Playwright installation.

## Run real sample acceptance

The test harness serves existing sample output; it does not build a substitute
executable. Install `playwright==1.63.0` into the Python interpreter used below.
Use installed Chrome on Windows (tested 153.0.8010.52). The local server supplies
COOP `same-origin` and COEP `require-corp` and closes with its isolated Chrome
profile after the run. This is build-directory testing, not delivery hosting.

```powershell
$runnerPython = (Get-Command python).Source
& $runnerPython -m pip install playwright==1.63.0
foreach ($configuration in @('debug', 'release')) {
    $sampleBuild = "build/web-minimal-$configuration"
    foreach ($scenario in @('success', 'missing-host', 'host-loss')) {
        & $runnerPython cmake/probes/minimal_app_web/run.py --build $sampleBuild --scenario $scenario
        if ($LASTEXITCODE -ne 0) { throw "Sample acceptance failed: $configuration/$scenario" }
    }
    & $runnerPython cmake/probes/minimal_app_web/run.py --build $sampleBuild --scenario runner-failure
    if ($LASTEXITCODE -ne 1) { throw 'Expected deliberately failing runner result' }
}
```

Append `--chrome '<absolute-chrome-path>'` for a different installation. The
`runner-failure` case first validates successful real sample execution, then
intentionally violates a runner expectation. Its JSON must report the deliberate
expectation error, application code 0 and runnerCode 1; an unrelated infrastructure
error is not equivalent evidence. Normal missing-host/host-loss checks expect
application code 1 but runnerCode 0 when all failure assertions pass.

Success measures 8–12 completed updates/second over five foreground seconds,
checks heartbeat and visible count, backgrounds/restores the actual Chrome tab,
clicks Stop and repeats Stop during cleanup. All cases check ordered snapshots,
terminal state, cleanup completion, no retained platform host, stable completed
count and reload-only retry. Real visibility uses CDP with Playwright focus
emulation disabled. Success writes `sample-running.png` and `sample-stopped.png`
beside the build cache for visual inspection. The test HTML is copied to
`bin/sample-acceptance.html`; it is not part of a staged runtime package.

## Host and lifecycle contract

Provide a connected `#nau-window` HTMLElement before loading the JavaScript, or
configure `Module.nauWindowSelector`. Read `Module.nauRuntime.status` and call
`Module.nauRuntime.stop()` from page controls after the bridge becomes available.
The harness displays state, completed updates, diagnostics and cleanup state,
retaining the final result after exit. It never synthesizes game updates in JavaScript.

The existing [lifecycle driver](browser_runtime_lifecycle.md) owns checked startup,
100 ms monotonic deadlines, visibility handling, Stop and cleanup. Browser dt is
bounded to 0–100 ms, hidden execution is best-effort, and the default foreground
progress deadline is 10 seconds. Failed remains terminal even after safe cleanup
finishes; another run requires page reload after failure or normal stop. No new
lifecycle API or sample fault-injection service is introduced.

## Build audits and native coexistence

With the SDK activated, after building both configurations:

```powershell
python cmake/probes/minimal_app_web/audit.py
python cmake/probes/emscripten/test_preflight.py
python cmake/probes/emscripten/test_platform_options.py
```

The sample audit requests CMake codemodel data and reconfigures the actual presets
with unavailable desktop dependency paths. It checks source/target selection,
all compile commands, final link options, runtime-file inventory, unsupported
configurations and unchanged native caches/source-tree desktop test wrapper.
Its JSON and configure logs are saved in each browser build directory.

In a native shell, without SDK activation:

```powershell
cmake --build --preset native-minimal-debug --target MinimalAppSample NauMinimalLifecycleTests
ctest --test-dir build/native-minimal-debug -C Debug --output-on-failure
cmake --build --preset native-minimal-release --target MinimalAppSample NauMinimalLifecycleTests
ctest --test-dir build/native-minimal-release -C Release --output-on-failure
```

Configure those native presets first if their directories do not exist. Both
sample builds and all 19 focused lifecycle/timing tests pass. Desktop profile
selection continues to use `app_sample_main.cpp`.

## Sample validation (before delivery integration)

On 2026-09-20, both actual sample configurations passed success, missing-host,
host-loss and deliberate runner-failure checks. Five-second rates were 9.7830 Hz
Debug and 9.7711 Hz Release, with 501 foreground heartbeats in each. Each build
audited 97 compile commands. See the parent repository's
`openspec/changes/minimal-app-web/validation.md` for full evidence.

The subsequent delivery integration adds default-build `MinimalAppWebPackage`
staging. See [package delivery](minimal_web_delivery.md) for the delivered page,
standalone server and copied-package acceptance. Fresh-checkout reproduction
remains separate from both browser harnesses.
