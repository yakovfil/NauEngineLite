[CmdletBinding()]
param(
    [switch]$Probe,
    [switch]$WithFmt,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug'
)

$ErrorActionPreference = 'Stop'
$expected = (Get-Content (Join-Path $PSScriptRoot 'emscripten-sdk-version.txt') -Raw).Trim()
$setup = "Expected external emsdk $expected. Run emsdk install $expected, emsdk activate $expected, then dot-source emsdk_env.ps1 in this PowerShell session."

function Invoke-ProbeCommand([string]$Command, [string[]]$Arguments) {
    # Windows PowerShell can treat redirected native warnings as errors. The
    # process exit code, not stderr output, determines configure/build success.
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Command @Arguments
        $result = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    if ($result -ne 0) { throw "$Command failed with exit code $result." }
}

try {
    if ($env:OS -ne 'Windows_NT') { throw 'This entry point requires Windows.' }
    if (-not $env:EMSDK) { throw "EMSDK is missing. $setup" }
    $sdk = Join-Path $env:EMSDK 'upstream/emscripten'
    $toolchain = Join-Path $sdk 'cmake/Modules/Platform/Emscripten.cmake'
    foreach ($file in @($toolchain, (Join-Path $sdk 'emcc.exe'), (Join-Path $sdk 'em++.exe'))) {
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing SDK file: $file. $setup" }
    }
    foreach ($compiler in @('emcc', 'em++')) {
        $command = Get-Command $compiler -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if (-not $command) { throw "$compiler is missing from PATH. $setup" }
        $compilerDirectory = (Get-Item -LiteralPath (Split-Path $command.Source)).FullName
        if ($compilerDirectory -ne (Get-Item -LiteralPath $sdk).FullName) {
            throw "Active $compiler is outside EMSDK: $($command.Source). $setup"
        }
        $output = (& $command.Source --version | Out-String)
        if ($LASTEXITCODE -ne 0) { throw "$compiler --version failed. $setup" }
        if ($output -notmatch 'Emscripten[^\r\n]*\s(\d+\.\d+\.\d+)') {
            throw "Unrecognized compiler identity: $output. $setup"
        }
        if ($Matches[1] -ne $expected) { throw "Detected $compiler $($Matches[1]). $setup" }
        Write-Host "$compiler $expected : $($command.Source)"
    }
    foreach ($tool in @('cmake', 'ninja')) {
        if (-not (Get-Command $tool -CommandType Application -ErrorAction SilentlyContinue)) {
            throw "$tool is missing from PATH. Install CMake (3.24+) and Ninja, then reopen PowerShell."
        }
        $toolVersion = (& $tool --version | Out-String)
        if ($LASTEXITCODE -ne 0) { throw "$tool --version failed." }
        Write-Host $toolVersion.Trim()
        if ($tool -eq 'cmake' -and ($toolVersion -notmatch 'cmake version (\d+\.\d+\.\d+)' -or [version]$Matches[1] -lt [version]'3.24.0')) {
            throw 'CMake 3.24 or newer is required for the probe and preset schema version 5.'
        }
    }
    Write-Host "SDK preflight passed. Use the web-minimal presets to build the sample; preflight alone does not validate application execution."
    if ($Probe) {
        $engine = Split-Path $PSScriptRoot
        $build = Join-Path $engine "build/emscripten-probe-$($Configuration.ToLowerInvariant())"
        if ($WithFmt) { $build += '-fmt' }
        Invoke-ProbeCommand cmake @('-S', (Join-Path $PSScriptRoot 'probes/emscripten'), '-B', $build, '-G', 'Ninja',
            "-DCMAKE_TOOLCHAIN_FILE=$toolchain", "-DCMAKE_BUILD_TYPE=$Configuration", '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            "-DNAU_PROBE_FMT=$WithFmt")
        Invoke-ProbeCommand cmake @('--build', $build, '--verbose')
        Write-Host "Coroutine/pthread compile-link probe passed ($Configuration). This is not a Nau application build or browser lifecycle test."
    }
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
