param([ValidateRange(1, 64)][int]$Jobs = 8)
$ErrorActionPreference = 'Stop'
$engine = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $engine 'engine/3rdparty_libs/USD-Fileformat-plugins'
$revision = & git -C $source rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $revision -ne '4045f510cfae94e06ed7964e0877f3a6d4fdf542') {
    throw 'Initialize the recorded USD-Fileformat-plugins submodule first.'
}
$sdk = Join-Path $engine 'engine/3rdparty_libs/build/openusd/install'
if (-not (Test-Path "$sdk/pxrConfig.cmake")) { throw 'Build OpenUSD first using cmake/build_openusd.ps1.' }
if (-not (Test-Path "$env:VCPKG_ROOT/vcpkg.exe")) { throw 'Set VCPKG_ROOT to a bootstrapped vcpkg checkout.' }
$build = Join-Path $engine 'engine/3rdparty_libs/build/usd-gltf-static'
& cmake -S "$PSScriptRoot/usd_gltf" -B $build -G 'Visual Studio 18 2026' -A x64 `
    "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
    '-DVCPKG_TARGET_TRIPLET=x64-windows-static-md' "-DNAU_USD_SDK_DIR=$sdk" "-DCMAKE_INSTALL_PREFIX=$sdk"
if ($LASTEXITCODE -ne 0) { throw 'glTF plugin configuration failed.' }
& cmake --build $build --config Release --target install --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'glTF plugin build/install failed.' }
& py -3.10 "$PSScriptRoot/usd_gltf/check_plugin.py" $sdk
if ($LASTEXITCODE -ne 0) { throw 'glTF plugin load/import validation failed.' }
Write-Output "USD glTF plugin installed into $sdk"
