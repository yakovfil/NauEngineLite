# Bundled fmt / Emscripten compatibility investigation

Date: 2026-09-19. The bundled header reports `FMT_VERSION 100202`. The one-line
fix below was applied after explicit user approval and verified against the
actual bundled sources. SDK 6.0.9 remains pinned.

## Findings

| Check | Result |
| --- | --- |
| Original fmt, Emscripten 6.0.9 / Clang 24 | `format.cc` and `os.cc` fail C++20 consteval checks |
| Original fmt, Emscripten 4.0.7 | `format.cc` reproduces the same pointer-subtraction diagnostic |
| Applied header fix, Emscripten 6.0.9 | Debug and Release compile/link both fmt sources with C++20, pthreads, exceptions and RTTI disabled |
| Applied fix, `fmt::format(FMT_STRING("value={}"), 42)` | Both configurations compile, link with `PROXY_TO_PTHREAD`, and return the expected string when run with SDK Node |
| Applied fix, integer format specifier with a string argument | Both configurations reject compilation with an invalid-format diagnostic |
| Native MSVC fmt target | Debug and Release library builds pass using the existing desktop configuration |

The failing diagnostic points to `base.h`'s `basic_format_parse_context::advance_to`:
`it - begin()` is not a constant expression. The format-string constructor creates
`str_` from `s`, then constructs its checker by converting `s` again. The diagnostic
and successful regression tests indicate that the parser and checker must share the
same string view instead of depending on separate conversions of `FMT_STRING`.

## Applied fix

In `engine/3rdparty_libs/fmt/include/fmt/base.h`, inside
`basic_format_string`'s consteval constructor:

```diff
-      detail::parse_format_string<true>(str_, checker(s));
+      detail::parse_format_string<true>(str_, checker(str_));
```

This retains C++20 and compile-time format validation. The local header
unconditionally defines `FMT_USE_CONSTEVAL`, so a command-line
`-DFMT_USE_CONSTEVAL=0` would be overwritten. Changing the dependency language
standard would also leave C++20 consumers exposed to its headers.

No SDK downgrade is proposed: 4.0.7 did not resolve the failure, and 6.0.9 remains
the exact pin. The comparison SDK was installed separately at `$HOME/emsdk-compat`
(release hash `ef4e9cedeac3332e4738087567552063f4f250d3`); the original 6.0.9
installation at `$HOME/emsdk` was preserved. Load the latter's `emsdk_env.ps1` for
normal project checks.

## Reproduce the regression checks

From the engine directory with SDK 6.0.9 loaded:

```powershell
./cmake/check_emscripten.ps1 -Probe -WithFmt -Configuration Debug
./cmake/check_emscripten.ps1 -Probe -WithFmt -Configuration Release
python cmake/probes/emscripten/test_fmt_compatibility.py
```

The additional probes have their own `build/emscripten-probe-debug-fmt` and
`build/emscripten-probe-release-fmt` directories. The Python check builds both,
runs the threaded probe and a `FMT_STRING` formatting check with SDK Node, and
verifies that an invalid format string still fails compilation. Logs are written
under a fresh `build/fmt-regression-*` directory.
The default probe remains available independently and exercises the production
browser build options with `xxHash` and C++20 coroutine/thread code.

Native regression commands (after desktop configuration):

```powershell
cmake --build build/desktop-platform-after --config Debug --target fmt
cmake --build build/desktop-platform-after --config Release --target fmt
```

This is SDK/dependency evidence, not Chrome or Nau lifecycle validation. Full
runtime module propagation still requires the separate minimal-runtime profile.

A related upstream [fmt consteval report](https://github.com/fmtlib/fmt/issues/4740)
describes similar compiler diagnostics and why command-line consteval overrides
are ineffective. The fix above was derived and tested against this checkout.
