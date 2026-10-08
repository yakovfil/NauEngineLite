# String migration checks

From the engine root, build and run the focused standard-string, Unicode,
formatting, hash, module-identity and version tests:

```powershell
cmake -S engine/core/kernel/tests/string_migration -B build/string-migration -G "Visual Studio 18 2026" -A x64
cmake --build build/string-migration --config Release --parallel 8
ctest --test-dir build/string-migration -C Release --output-on-failure
```

Repeat the build and CTest commands with Debug. For Web, activate the recorded
Emscripten SDK, then configure the same source with Ninja, a separate build
directory, `-DCMAKE_BUILD_TYPE=Release` and
`-DCMAKE_TOOLCHAIN_FILE=$env:EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake`.
Build that directory and run CTest; the activated SDK Node runtime executes the
exception-disabled tests. The fixture records separate 32-bit and 64-bit
identifier hash answers.
