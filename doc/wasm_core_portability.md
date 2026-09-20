# Wasm core validation

The production minimal core builds and runs focused tests on wasm32 without a
browser window implementation. This is the `wasm-core-portability` milestone,
not a runnable browser application or a staged web package.

## Tools and builds

Use the recorded bundled sources, CMake 3.24+, Ninja and activated emsdk **6.0.9**.
The validation entry selects the SDK's Node through `EMSDK_NODE`, not a system
Node found on PATH. Python 3 runs diagnostic/audit helpers using only its standard
library. No vcpkg, Qt, USD, generator packages or additional source downloads are
needed. Native validation additionally needs VS 2022 C++ tools and a Windows SDK.
See the parent repository's `prerequisites.md` for installation details.

From `NauEngineLite`, in PowerShell:

```powershell
. "$HOME/emsdk/emsdk_env.ps1"
foreach ($configuration in @('Debug', 'Release')) {
    $buildDir = "build/wasm-core-$($configuration.ToLower())"
    cmake -S cmake/probes/wasm_core -B $buildDir -G Ninja "-DCMAKE_TOOLCHAIN_FILE=$env:EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" "-DCMAKE_BUILD_TYPE=$configuration" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    cmake --build $buildDir --parallel 4
    ctest --test-dir $buildDir --output-on-failure
}
```

Use a separate native shell (SDK activation is unnecessary):

```powershell
foreach ($configuration in @('Debug', 'Release')) {
    $buildDir = "build/core-native-$($configuration.ToLower())"
    cmake -S cmake/probes/wasm_core -B $buildDir -G 'Visual Studio 17 2022' -A x64 "-DCMAKE_CONFIGURATION_TYPES=$configuration"
    cmake --build $buildDir --config $configuration --target NauCorePortabilityTests
    ctest --test-dir $buildDir -C $configuration --output-on-failure
}
```

Targets are `NauKernel`, `NauFrameworkMinimal` and `NauCorePortabilityTests`.
The framework's eight production sources compile against the real PlatformApp
headers. Its global-property implementation is exercised by core tests; the
application is not linked. `PlatformAppApi` is a header interface, with no fake
PlatformApp implementation. A small CoreTest module uses the production static
registration generator and module manager.

Wasm outputs are `bin/NauCorePortabilityTests.js` and `.wasm`; these are test
artifacts, not browser application packages. CTest runs 27 focused tests, including
16 existing math serialization cases, on a pthread worker. A separate fatal
diagnostic subprocess must print its diagnostic and exit nonzero. Tests have
60-second and 40-second process limits. Native runs 26 applicable tests.

## Contracts and deliberate boundaries

- The manifest shares 62 common kernel sources, selecting nine Windows backend
  sources or three Emscripten sources. Bundled dependencies remain source-built.
- C++20 coroutines use compiler feature detection. All Wasm compilation uses the
  threaded ABI and disables exceptions/native RTTI. Executables use
  `PROXY_TO_PTHREAD` and a 1 MiB stack: global-property helpers alone reserve a
  64 KiB local allocator, exceeding the SDK's default stack with caller frames.
- Allocation tests cover alignment, odd sizes, growth/shrink, refreshed allocation
  headers, cross-thread freeing and storage surviving its producing thread.
- UID parsing preserves canonical lowercase text and case-insensitive hex input,
  equality and equal-value hashing. Concurrent generation uses the SDK random
  facility through `std::random_device`, serialized by a mutex. Hash numbers and
  raw UID storage are not a cross-platform serialized format.
- Checked Wasm UTF conversion reports a diagnostic in Debug and Release and
  returns an empty result for malformed input, without a converted prefix.
  Native checked conversions retain their exception behavior. Valid BMP,
  supplementary characters and target-width wide strings round-trip.
- The timer backend uses one worker, `steady_clock` deadlines and a locked queue.
  Callbacks run outside the queue lock. Cancellation suppresses queued callbacks;
  an entry already claimed by the worker may finish once. Disposal invokes
  uncancelled simple callbacks and reports cancellation to delayed task callbacks,
  matching the native shutdown contract. Runtime shutdown accounts for queued
  executor work and pending completions before joining/releasing owned workers.
- Event reset modes, recursive critical sections, timed locking on Wasm, worker
  coordination, thread-local cleanup and callback reentry are exercised.
- Scalar vector/matrix/SoA operations and existing JSON math representations are
  retained; the added numerical checks use a `1e-5` tolerance.
- Wasm diagnostics use stderr without Windows dialogs. Native host file streams,
  temporary host paths and host known folders report unavailability and return
  empty/null results. `KnownFolder::Current` returns the real SDK filesystem
  working directory, not a host-folder substitute. Virtual filesystem creation
  and in-memory global properties remain functional. No persistent filesystem
  or general native disk backend is provided.

## Evidence and audits

Validated on 2026-09-19 from engine base commit
`52197201e525a584fb9490be22b1d064b0c666e1` plus the uncommitted minimal-profile,
Emscripten and core-portability changes. Tools: CMake 4.2.1, Ninja 1.13.2,
emsdk 6.0.9, SDK Node 24.19.0, MSVC 19.44.35229.0 and Windows SDK 10.0.26100.0.

Both isolated native core configurations pass. Both Wasm configurations pass the
core and fatal-diagnostic CTest entries. Native minimal lifecycle Debug/Release
also pass with real PlatformApp startup/stepping/shutdown. Representative desktop
`NauFramework` and `PlatformApp` Debug libraries compile with
`/p:BuildProjectReferences=false`; this is not a full desktop dependency build or
application link. Desktop regeneration needs Python 3.10 and its Scripts directory
on PATH so the existing `pip show cymbal/clang` checks select the installed packages.

The entry creates a CMake file-API query. For a fresh directory, repeat its
configuration once to populate that query before running the graph audit:

```powershell
python cmake/probes/wasm_core/audit.py --native build/core-native-debug --wasm build/wasm-core-debug --wasm build/wasm-core-release
python cmake/probes/wasm_core/audit.py --native build/core-native-release --wasm build/wasm-core-debug --wasm build/wasm-core-release
python cmake/probes/wasm_core/verify_configuration.py
python cmake/probes/test_minimal_runtime.py --minimal build/native-minimal-debug --minimal build/native-minimal-release --desktop build/desktop-platform-after
```

Run `verify_configuration.py` with the SDK active. The final audit needs the
previously configured native lifecycle/desktop directories described in
[minimal_runtime_profile.md](minimal_runtime_profile.md). Audits confirm common
source parity, 98 Wasm compile commands, no Windows definitions/native compiler
flags, missing-SDK/C++17 rejection, and minimal graphs reduced to 18 targets.

The browser presets now build the [actual minimal sample](minimal_app_web.md)
with production platform services and the shared worker lifecycle. Sample Chrome
acceptance is recorded separately; runtime staging, relocation and fresh-checkout
delivery reproduction remain pending in `emscripten-build` and delivery work.
