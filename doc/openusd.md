# Local OpenUSD SDK

The upstream repository is https://github.com/PixarAnimationStudios/OpenUSD.git.
It is recorded in `.gitmodules` at `engine/3rdparty_libs/OpenUSD`, pinned to
**v24.08**, commit `59992d2178afcebd89273759f2bddfe730e59aa8`, matching Nau's
existing USD integration. Initialize the recorded revision with:

```powershell
git submodule update --init engine/3rdparty_libs/OpenUSD engine/3rdparty_libs/USD-Fileformat-plugins
```

From NauEngineLite, build the Windows x64 Release SDK with:

```powershell
py -3.10 -m pip install Jinja2==3.1.6
./cmake/build_openusd.ps1
```

The helper requires Visual Studio 2026 C++ tools, CMake 4.2+, Python 3.10,
Git, curl, and a bootstrapped vcpkg checkout selected by `VCPKG_ROOT`. It enables shared libraries, Python bindings/schema tools and
MaterialX, with imaging/Hydra and usdview disabled. `-Jobs` controls parallelism
(default 8).

Sources remain in the upstream submodule. Downloaded dependencies, intermediate
files, logs and installed binaries go under the ignored
`engine/3rdparty_libs/build/openusd/` directory. The SDK install prefix is
`engine/3rdparty_libs/build/openusd/install/`.

The editor also requires Adobe's `usdGltf_plugin`, which is not part of Pixar's
core SDK. Its source URL is https://github.com/adobe/USD-Fileformat-plugins.git,
recorded at `engine/3rdparty_libs/USD-Fileformat-plugins`, pinned to v1.0.7 commit
`4045f510cfae94e06ed7964e0877f3a6d4fdf542`. The SDK helper builds this plugin after
OpenUSD. To add or rebuild only this plugin in an existing SDK, run:

```powershell
./cmake/build_usd_gltf.ps1
```

Only the glTF/glb format is enabled. OpenImageIO and zlib dependencies
are selected by `cmake/usd_gltf/vcpkg.json`. TinyGLTF 2.8.21 is fetched at Git revision
`d9c2bbe90c6af6fca5e2d3f8f8e0091e0a913e48`; the corresponding vcpkg archive hashes
no longer match upstream downloads. Build files and dependencies stay under
the ignored `engine/3rdparty_libs/build/usd-gltf-static/` directory. Plugin metadata and
the DLL are installed into the SDK's `plugin/usd/`, with `fileformatUtils.dll` in `bin/`.
Third-party dependencies use `x64-windows-static-md`, retaining the dynamic CRT
while avoiding runtime DLL name collisions with engine dependencies such as fmt.
The helper verifies plugin loading and triangle import from both `.gltf` and `.glb`.
The editor copies these files into its deployment and rejects a local SDK without
the glTF plugin during configuration.

The upstream v24.08 helper uses an obsolete Boost JFrog URL. Our helper obtains
the same Boost 1.78 archive from
https://archives.boost.io/release/1.78.0/source/boost_1_78_0.zip and verifies
SHA256 `f22143b5528e081123c3c5ed437e92f648fe69748e95fa6e2bd41484e2986cc3`.
Boost's older build system uses its `msvc-14.3` toolset label with explicit paths
to the actual VS 2026 compiler and environment; that label does not mean VS 2022
compiled the libraries. The helper also supplies the CMake policy compatibility
setting needed by the older dependencies. It does not patch upstream sources.

For command-line tools, select the same Python 3.10 used to build the SDK:

```powershell
$usdSdk = (Resolve-Path engine/3rdparty_libs/build/openusd/install).Path
$usdPython = py -3.10 -c 'import sys; print(sys.prefix)'
$env:PATH = "$usdPython;$usdSdk/bin;$usdSdk/lib;$env:PATH"
$env:PYTHONPATH = "$usdSdk/lib/python"
usdGenSchema.cmd --help
```

The generated `.cmd` launchers resolve `python` from PATH. Running them with a
different Python major/minor version cannot load these Python 3.10 bindings.

Validated on 2026-10-06 with VS 2026/MSVC 19.51, CMake 4.2.1 and Python 3.10.11:

- Release SDK build and repeat invocation of `cmake/build_openusd.ps1` passed.
- Python `Usd`, `UsdGeom` and `Sdf` imports and a scene save/load round trip passed.
- `usdGenSchema.cmd` generated C++ classes, plugin metadata and schema data.
- A separate C++20 Release CMake consumer linked against `usd`/`usdGeom` and ran.
  It used `NOMINMAX`, `WIN32_LEAN_AND_MEAN`, and `BOOST_ALL_NO_LIB`, with explicit
  imported-target linkage.

Local build logs are under `engine/3rdparty_libs/build/openusd/`; smoke-check
sources and logs are in its `validation/` directory. These are generated evidence,
not committed build outputs. The editor Release build and deployment also passed
using this SDK on 2026-10-06. The glTF plugin loaded and imported triangle meshes
from `.gltf` and `.glb` in both the SDK and deployed editor. All nine editor-required
USD plugins loaded from the deployment, and a fresh editor process reached its
responsive project browser. Full project editing and Debug were not validated.

To repeat the deployed plugin check from NauEngineLite:

```powershell
py -3.10 cmake/usd_gltf/check_plugin.py engine/3rdparty_libs/build/openusd/install --deployment <editor-output-directory>
```

This upstream install layout differs from Nau's previous prebuilt
`usd24.08_no_Hydra.zip`: Python is external, and libraries/plugins use the
upstream directory layout. Configure consumers with
`-DNAU_USD_SDK_DIR=<absolute-install-prefix>` to select this layout without
downloading the legacy package. See the editor README for the full Release
configure command. Omitting this option retains the legacy package route.
