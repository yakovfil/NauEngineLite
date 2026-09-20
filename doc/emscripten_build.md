# Emscripten toolchain setup and validation

Current status: external SDK **6.0.9** builds the actual minimal Nau sample in
Debug and Release. Both browser presets pass real Chrome sample acceptance using
the production shared lifecycle and static PlatformApp. See
[minimal_app_web.md](minimal_app_web.md) for sample build/test commands and results.
Minimal runtime packages are staged and tested separately. The scene-driven
[NauDemo browser guide](naudemo_web.md) describes the new demo presets, WebGL 2
requirements, complete packages and fresh-checkout acceptance.

## Install and activate explicitly

Use Windows PowerShell. Git, CMake 3.24+ and Ninja must be on PATH. Tested versions
are Git 2.55.0.windows.5, CMake 4.2.1 and Ninja 1.13.2. For a new external SDK:

```powershell
winget install --id Ninja-build.Ninja --exact --version 1.13.2
git clone https://github.com/emscripten-core/emsdk.git "$HOME/emsdk"
& "$HOME/emsdk/emsdk.bat" install 6.0.9
& "$HOME/emsdk/emsdk.bat" activate 6.0.9
```

Reuse the existing installation if it is already present. Reopen PowerShell after
installing Ninja. From the engine directory, activate the environment and check it:

```powershell
. "$HOME/emsdk/emsdk_env.ps1"
./cmake/check_emscripten.ps1
```

Use the actual external SDK location if installed elsewhere. The scripts contain
no workstation-specific paths. Installation brings its own Node 24.19.0, Python
3.13.3, LLVM and sysroot. SDK release hash:
`f04ea239d533260dd1db760dd2d668d5f9a88d6b`.

## Reproduce the capability checks

Run from `NauEngineLite`:

```powershell
./cmake/check_emscripten.ps1 -Probe -Configuration Debug
./cmake/check_emscripten.ps1 -Probe -Configuration Release
python cmake/probes/emscripten/test_preflight.py
python cmake/probes/emscripten/test_platform_options.py
```

The first two commands configure separate Ninja directories at
`build/emscripten-probe-debug` and `build/emscripten-probe-release`, then compile
and link the standalone probe plus the recorded bundled C library `xxHash` using
the production `nau_add_compile_options` helper. Each produces `nau_emscripten_probe.js` and
`nau_emscripten_probe.wasm`. This SDK emits no separate worker file for the probe.
These files are probe artifacts, not a staged Nau runtime package.

Both compile and link use `-pthread`; the application link uses
`-sPROXY_TO_PTHREAD=1`. Compilation uses C++20 without exceptions or RTTI. Debug
uses `-O0`, `-g3` and assertions; Release uses `-O3`. The source exercises a resumable
coroutine, atomics, and a joined thread. No browser execution is claimed.

The preflight test verifies seven SDK contract cases with logs in a fresh
`build/emscripten-contract-*` directory. Mismatch tests change expected-version
metadata in a disposable fixture, leaving the installed SDK intact. It also
checks that `.emscripten` activation metadata is unchanged. Direct browser CMake
configuration invokes the SDK guard before `project()`; native configuration does
not require SDK discovery.

The platform test checks actual preset resolution with inaccessible desktop
dependency paths, compiles the C/C++ probe in both configurations, and audits
generated compile/link commands for pthread support and absence of Windows/x86
definitions or MSVC-only flags. It is not a real-runtime acceptance test.

## Browser presets

Run from the engine directory after SDK environment setup:

```powershell
cmake --preset web-minimal-debug
cmake --preset web-minimal-release
```

Both configurations now generate the production minimal sample graph. They skip
desktop dependency discovery and source-tree desktop test-wrapper generation.
Unsupported browser desktop-profile/shared-library settings fail explicitly.

The presets use Ninja and independent `build/web-minimal-debug` and
`build/web-minimal-release` caches. Each has its own `bin`, `lib` and `install`
paths. They select `NAU_RUNTIME_PROFILE=minimal`, `wasm32`, static linking and
the external Emscripten toolchain;
neither inherits desktop `config_base` or its Windows SDK/vcpkg settings.
Build the actual sample with the matching presets:

```powershell
cmake --build --preset web-minimal-debug
cmake --build --preset web-minimal-release
```

Both builds produce `bin/MinimalAppSample.js` and `.wasm`. Pthread compile/link
options flow through the shared helper to each selected source target; the sample
selects `PROXY_TO_PTHREAD`, runtime exit, a 12-worker pool and no blocking on the
browser UI thread. Run `python cmake/probes/minimal_app_web/audit.py` after building
both configurations to audit the real graph and its 97 compile commands.

## Desktop coexistence validation

The existing `win_vs2022_x64` preset configured successfully without `EMSDK` in
separate `build/desktop-platform-before` and `build/desktop-platform-after`
directories. Its preset definitions and 36 selected compiler/cache settings
matched. The platform test verified that browser configuration left the original
native cache untouched. To repeat the comparison with those directories present:

```powershell
python cmake/probes/emscripten/test_platform_options.py --native-before build/desktop-platform-before --native-after build/desktop-platform-after
```

Desktop configuration resolved the manifest's recorded DXC and protobuf packages.
The affected desktop `NauFramework` and `PlatformApp` Debug libraries also compiled
with dependency project builds skipped. Full desktop application linking remains
unverified. Native minimal Debug/Release builds and lifecycle tests pass; see
[the profile guide](minimal_runtime_profile.md).

CMake 4.2.1 produces an SDK warning about shared-library support. The probe and
browser application use static linking. A native VS 2022 C++20 smoke test
also configured, built and ran with MSVC v143 and Windows SDK 10.0.26100.0;
full desktop application regression remains pending.

## Integration and delivery boundary

The [Wasm core milestone](wasm_core_portability.md) now builds production NauKernel,
compiles NauFrameworkMinimal and passes focused Debug/Release tests under SDK
Node 24.19.0. That guide contains the independent core build commands, supported
contracts and graph audits. Browser platform services have separate Chrome
acceptance evidence. The actual sample presets now build and pass Chrome lifecycle
acceptance; [package delivery](minimal_web_delivery.md) now supplies staged folders
and relocated browser acceptance.

The optional `-WithFmt` probe passes with the approved bundled fmt consteval fix.
See the [compatibility investigation](fmt_emscripten_compatibility.md) for the
SDK comparison and Debug/Release/native regression evidence. Run
`python cmake/probes/emscripten/test_fmt_compatibility.py` to reproduce the
Wasm build, runtime formatting and invalid-format rejection checks.

Initialize the recorded engine submodules with
`git submodule update --init --recursive -- NauEngineLite` from the parent root
on a fresh checkout. Do not change dependency revisions to satisfy a browser
build, and do not reuse desktop vcpkg libraries or native caches.

The implemented profile defines `NauFrameworkMinimal`, `NauKernel`, `PlatformApp`
and exact dependency/module lists in `cmake/NauMinimalRuntime.cmake`. Browser
sample graph generation and real application execution are validated with SDK 6.0.9.
Default builds produce `build/web-minimal-debug/package` and
`build/web-minimal-release/package`. Each contains the actual JS/Wasm, index.html,
serve.py, README.md and a configuration/SDK/SHA-256 manifest. Staging replaces
obsolete owned package contents and fails on missing inputs. SDK 6.0.9 embeds
worker support in its loader; no separate worker file is required.

Run `python build/web-minimal-debug/package/serve.py --port 8000` or the equivalent
Release path and open the printed URL. Copying the complete package elsewhere
preserves execution, including nested URL bases. Ordinary serving needs only
Python 3.9+ and current Chrome. Fresh-checkout/environment build reproduction
remains pending; clean build-directory and relocation checks do not establish it.

References: [SDK setup](https://emscripten.org/docs/getting_started/downloads.html)
and [pthread support](https://emscripten.org/docs/porting/pthreads.html).
