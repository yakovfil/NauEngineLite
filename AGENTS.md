# Repository Guidelines

## Project Structure & Module Organization

Nau Engine is a C++20 game engine. `engine/core/kernel/` contains foundational APIs, implementations, and tests in `include/`, `src/`, and `tests/`. Feature systems live in `engine/core/modules/`; application support lives in `engine/core/app_framework/`. `tools/` contains asset, shader, project, and build utilities. The sibling `NauSamples/` checkout contains example applications selected through `NAU_SAMPLES_SOURCE_DIR`; `project_templates/` contains starter projects and resources. Shared build logic lives in `cmake/`, documentation in `doc/`, and bundled dependencies in `engine/3rdparty_libs/`.

## Build, Test, and Development Commands

For the standard Windows build, install Visual Studio 2022 with C++ tools, Python, and CMake with preset version 5 support. Bootstrap vcpkg and set `VCPKG_ROOT` to its checkout.

- `git submodule update --init --recursive`: initialize dependencies.
- `cmake --preset win_vs2022_x64`: configure into `build/win_vs2022_x64/`.
- `cmake --build --preset "VS Debug"`: build the Debug configuration, including enabled tools, samples, and tests.
- `ctest --test-dir build/win_vs2022_x64 -C Debug --output-on-failure`: run registered tests.
- `cmake --install build/win_vs2022_x64 --config Debug`: install the SDK into `output/`.

Use the generated Visual Studio solution to select and launch a sample. The optional `win_ninja_clangcl-debug` preset supports Ninja with clang-cl.

## Coding Style & Naming Conventions

Follow `doc/coding_style_guide.md` and `.clang-format`: four spaces, no tabs, Allman braces, and no fixed line-length limit. Format changed C++ files with `clang-format -i <file>`. Use `snake_case` filenames and namespaces, `PascalCase` classes, `camelCase` functions, and `NAU_`-prefixed uppercase macros. Prefer EASTL where available; avoid exceptions and native C++ RTTI. Follow the documented copyright-header convention for new headers.

## Testing Guidelines

Tests use GoogleTest with CTest discovery. Keep regression tests beside the relevant subsystem under `tests/`, using `test_*.cpp` filenames and descriptive `TEST`/`TEST_F` cases. `NAU_CORE_TESTS` defaults to `ON`. Build before running CTest; use `-R <regex>` to focus on relevant cases. No numerical coverage threshold is documented; cover changed behavior and failure paths.

## Commit & Pull Request Guidelines

History favors short descriptive subjects, such as `Miniaudio build fix` and `Add submodules`, without a consistent Conventional Commits prefix. Keep commits focused. For pull requests, describe the problem, resulting behavior, affected modules, and validation commands/results. Link relevant issues and include screenshots for visible rendering or UI changes.
