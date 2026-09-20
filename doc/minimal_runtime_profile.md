# Minimal runtime profile

`NAU_RUNTIME_PROFILE=minimal` builds the existing application lifecycle without
scene, Graphics, UI, VFX or ImGui integration. The default `desktop` profile keeps
the full framework. Link one framework variant per process.

## Native Windows build

Install VS 2022 C++ tools with a Windows SDK and CMake supporting preset schema 5
(3.24 or newer). Initialize the recorded bundled sources. No vcpkg, Qt, USD,
Emscripten or Python generator packages are required by this runtime profile.
Python is needed to run the graph audit below; bundled GoogleTest may discover it
optionally during configuration.

From `NauEngineLite` in PowerShell:

```powershell
cmake --preset native-minimal-debug
cmake --build --preset native-minimal-debug
ctest --test-dir build/native-minimal-debug -C Debug --output-on-failure
cmake --preset native-minimal-release
cmake --build --preset native-minimal-release
ctest --test-dir build/native-minimal-release -C Release --output-on-failure
```

Each preset has its own build and install directories. Executables appear in
`build/native-minimal-<configuration>/bin/Debug` or `bin/Release`. Presets build
`MinimalAppSample` and `NauMinimalLifecycleTests`. The sample uses the existing
`app_sample_main_min.cpp` loop and runs until the application requests stop.
CTest runs the bounded lifecycle test rather than that indefinite sample loop.

## Selected sources

`cmake/NauMinimalRuntime.cmake` owns the dependency and module lists. The required
module set is `NAU_MINIMAL_MODULES=PlatformApp`; unsupported selections fail.
Targets are `NauKernel`, `NauFrameworkMinimal`, `PlatformApp` and `PlatformAppApi`.
The sample and tests reuse generated static module registration.

Bundled directories: EABase, EASTL, tiny-utf8, fmt, utfcpp, ModifiedSonyMath,
jsoncpp, wyhash and fast_float. Focused tests additionally use googletest.
The explicit common/platform manifest in `NauMinimalKernelSources.cmake` now
excludes proven-unused compression and Dagor IO groups; see
[source-selection evidence](minimal_kernel_sources.md). Missing required dependency
CMake files fail with a source-restoration diagnostic; no replacement is fetched.

The framework explicitly compiles application_impl/setup, background_work_service,
global_properties_impl, logging_service, platform_window_service,
main_loop_service and concurrent_execution_container. Private profile guards
exclude renderer integration in the framework and Windows window manager.
Startup, work queues, service lifecycle and multi-phase shutdown remain shared.

## Validation on 2026-09-19

Both native configurations built and passed `MinimalRuntime.Lifecycle` with
MSVC 19.44.35229.0, Windows SDK 10.0.26100.0 and CMake 4.2.1. The test checks real
core/window services, five ordered pre/post update pairs, service shutdown,
destruction and application completion. CTest limits execution to 30 seconds.

The original profile had 23 targets; after the verified core source/dependency
refinement, both configured graphs contain 18 targets, including CMake utilities, with
no rendering modules, shader/content tools or unrelated samples/tests. The
existing desktop preset still selects its full framework and the original sample
entry point. Its `NauFramework` and `PlatformApp` Debug static libraries compiled
with `BuildProjectReferences=false`; this validates the affected desktop sources,
not a full desktop application link or test run.

To reproduce graph and failure audits, create an empty
`.cmake/api/v1/query/codemodel-v2` file inside each build directory and reconfigure.
Then run:

```powershell
python cmake/probes/test_minimal_runtime.py --minimal build/native-minimal-debug --minimal build/native-minimal-release --desktop build/desktop-platform-after
```

The desktop directory is an existing `win_vs2022_x64` configuration with that
file-API query enabled; it requires the normal desktop prerequisites. The audit
also tests unsupported-module and missing-source diagnostics in separate fixtures
without removing dependencies from the working checkout.

The separate [Wasm core validation](wasm_core_portability.md) now passes in Debug
and Release. Browser platform services, application worker lifecycle and delivery
validation remain prerequisites of the [Emscripten build](emscripten_build.md).
