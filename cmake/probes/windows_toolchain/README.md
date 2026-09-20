# VS 2026 compiler validation

`test_toolchain.py` checks the native compiler guard. `failed-tests-release.txt`
and `failed-tests-clang-debug.txt` / `failed-tests-clang-dll-debug.txt` contain exact, user-approved temporary runtime
test exclusions; the root `failed-tests.md` records reasons and outcomes.

## clang-cl 22 dependency preparation

The retained Windows clang-cl route uses LLVM 22.1.3 with the VS 2026 v145 STL
and Windows SDK. Configure the DLL route with the exact USD 24.08 package selected
through `FETCHCONTENT_SOURCE_DIR_USD`, then prepare its headers before building:

```powershell
python cmake/probes/windows_toolchain/prepare_clang_dependencies.py --usd-root C:/Tools/usd24.08 --googletest-root engine/3rdparty_libs/googletest
```

Static builds without USD need only `--googletest-root`. Preparation is idempotent,
preserves line endings, rejects unknown header contents before making changes,
and saves original headers with `.nau-clang.bak` suffixes. Keep those backups out
of commits; restoring them reverses preparation. This is dependency setup and
does not change package versions or binaries. Google Test is a Git submodule;
its prepared header will appear modified until restored.

The changes are deliberately narrow:

- Under Clang, omit USD's uninstantiable proxy assignment, which calls a missing
  `_Set` method. [Upstream USD removed the same operator](https://github.com/PixarAnimationStudios/OpenUSD/blob/dev/pxr/usd/sdf/childrenProxy.h).
- Under Windows Clang, give TBB's context-kind enum the explicit `int` representation
  used by its Windows producer. This permits the library's existing internal
  states 2–4 without an out-of-range constant expression. MSVC preprocessing is
  unchanged. A compiler probe verifies both compilers use a four-byte enum and
  a 256-byte context with eight-byte alignment; linked runtime checks remain
  necessary to establish compatibility beyond layout.
- Make Google Test's numerical `char8_t` printing cast explicit. This preserves
  its code-unit value and avoids the LLVM 22 implicit-character-conversion error.

USD's imported CMake targets already name the producer's Boost libraries. For
Clang consumers, the engine disables Boost's filename-guessing auto-link feature
and uses those explicit imported dependencies instead.

Run preparation regression checks with:

```powershell
python -m unittest discover -s cmake/probes/windows_toolchain -p test_prepare_clang_dependencies.py
```
