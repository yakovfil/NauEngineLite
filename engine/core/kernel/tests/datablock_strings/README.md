# DataBlock string boundary checks

These native tests link a rebuilt Release DLL SDK and exercise text loading,
copying and includes without a source filename. They cover nullable text paths
used by the shader compiler, plus existing DataBlock value access.

From the engine root:

```powershell
$sdk = 'C:/path/to/rebuilt-or-relocated-sdk'
cmake -S engine/core/kernel/tests/datablock_strings -B build/datablock-strings -G "Visual Studio 18 2026" -A x64 -DNAU_ENGINE_SDK_DIR=$sdk
cmake --build build/datablock-strings --config Release --parallel 8
$env:PATH = "$sdk/bin/Release;$env:PATH"
ctest --test-dir build/datablock-strings -C Release --output-on-failure
```

Keep the matching SDK DLLs together. The fixture uses bundled GoogleTest sources
for the test executable and imported SDK targets for the engine.
