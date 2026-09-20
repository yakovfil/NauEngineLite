# External sample sources

Samples live in the sibling NauSamples submodule. Initialize it from the parent Nau checkout:

```powershell
git submodule update --init NauSamples
Set-Location NauEngineLite
cmake --preset win_vs2022_x64
cmake --build build/win_vs2022_x64 --config Debug --target SceneBaseSample
```

Existing presets and sample targets are retained. Source paths use `NAU_SAMPLES_SOURCE_DIR`, defaulting to `../NauSamples` relative to the engine; binary paths remain under the engine build directory, including `samples/<sample>` intermediate files and `bin/<configuration>` native executables.

For a relocated sample checkout, configure with a quoted absolute path:

```powershell
cmake --preset win_vs2022_x64 -B build/custom-samples "-DNAU_SAMPLES_SOURCE_DIR=D:/Projects/My Samples"
```

After extraction, use a clean build directory or remove only the obsolete cache through your normal build workflow and reconfigure. Do not recreate `NauEngineLite/samples` or copy old ignored shader caches. Native source builds resolve assets through the selected sample root. Rebuild after relocating it. NauDemo's native integration uses the same selected root; its fast build helper accepts `--samples` and otherwise reads the engine cache.

The minimal browser presets require NauSamples. Activate the pinned SDK 6.0.9, then use the existing `web-minimal-debug` and `web-minimal-release` configure/build presets. Packages and serving commands are unchanged. Browser NauDemo uses its own assets and does not require NauSamples for its demo-only graph. Engine-only configurations with no sample consumer likewise do not require the checkout. Missing required sample inputs fail configuration with setup guidance; CMake never initializes Git repositories implicitly.

Native SDK installation distributes sample contents under `<prefix>/samples`. The installed SDK CMake entry discovers that tree and builds shared sample support before consumers. It uses installed resource paths rather than the original sample source location.

Build the native targets first, including the additional libraries and tools consumed by installation:

```powershell
cmake --build build/win_vs2022_x64 --config Debug --target SceneBaseSample inputDemo ScriptsSample ozz_options ozz_animation_offline ozz_animation_tools gltf2ozz ozz_geometry gmock gmock_main gtest_main Audio Shared
cmake --install build/win_vs2022_x64 --config Debug
```

The existing desktop preset installs into `NauEngineLite/output`. Use that configured installation directory; some existing SDK install rules use absolute destinations and cannot be redirected consistently with `cmake --install --prefix`. Copy the complete output directory to relocate the SDK. From its new directory:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target MinimalAppSample SceneBaseSample inputDemo ScriptsSample
./bin/Debug/SceneBaseSample.exe
```

Keep the SDK root as the working directory when launching its samples so installed resource discovery can find `samples/<name>`. The original source checkouts are not required. Use a matching configuration for the installed libraries and sample build.

The parent repository records compatible engine, samples and demo commits. If publishing, make these component commits available in their repositories before pushing the parent gitlinks. A successful local checkout test does not establish remote commit availability. Extraction provenance and executed validation are recorded in `openspec/changes/archive/2026-09-20-extract-nau-samples/validation.md` in the parent repository.
