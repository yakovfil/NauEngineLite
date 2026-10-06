param([ValidateRange(1, 64)][int]$Jobs = 8)

$ErrorActionPreference = 'Stop'
$engine = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $engine 'engine/3rdparty_libs/OpenUSD'
$build = Join-Path $engine 'engine/3rdparty_libs/build/openusd'
$install = Join-Path $build 'install'
$revision = & git -C $source rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $revision -ne '59992d2178afcebd89273759f2bddfe730e59aa8') {
    throw 'Initialize the recorded OpenUSD v24.08 submodule before building.'
}

$python = & py -3.10 -c 'import sys; print(sys.executable)'
if ($LASTEXITCODE -ne 0) { throw 'Python 3.10 is required.' }
& $python -c 'import jinja2'
if ($LASTEXITCODE -ne 0) { throw 'Install the schema-generator dependency: py -3.10 -m pip install Jinja2==3.1.6' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
$vs = & $vswhere -latest -version '[18.0,19.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vs) { throw 'Visual Studio 2026 C++ tools are required.' }
$vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
# Import the compiler environment into this build process only.
$environment = & cmd.exe /d /c "call `"$vcvars`" >nul && set"
if ($LASTEXITCODE -ne 0) { throw 'Could not initialize the VS x64 environment.' }
foreach ($line in $environment) {
    if ($line -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
# Boost 1.78 recognizes the VS 2022 environment name. Point it to the active
# VS 2026 compiler; this does not change the selected compiler or upstream files.
$env:VS170COMNTOOLS = Join-Path $vs 'Common7/Tools/'

New-Item -ItemType Directory -Force (Join-Path $install 'src') | Out-Null
$pythonRoot = Split-Path $python
$compiler = (Get-Command cl.exe).Source.Replace('\', '/')
$boostConfig = Join-Path $build 'boost-config.jam'
@"
using msvc : 14.3 : "$compiler" : <setup>"$($vcvars.Replace('\', '/'))" ;
using python : 3.10 : "$($python.Replace('\', '/'))" : "$($pythonRoot.Replace('\', '/'))/include" : "$($pythonRoot.Replace('\', '/'))/libs" ;
"@ | Set-Content -LiteralPath $boostConfig
$boost = Join-Path $install 'src/boost_1_78_0.zip'
$boostHash = 'f22143b5528e081123c3c5ed437e92f648fe69748e95fa6e2bd41484e2986cc3'
# The upstream script's JFrog URL no longer serves this archive. Prepopulate its
# cache from Boost's current archive host, verified against its published SHA256.
if (-not (Test-Path $boost) -or (Get-FileHash $boost -Algorithm SHA256).Hash -ne $boostHash) {
    $download = "$boost.download"
    & curl.exe --fail --location --retry 2 --output $download 'https://archives.boost.io/release/1.78.0/source/boost_1_78_0.zip'
    if ($LASTEXITCODE -ne 0) { throw 'Boost download failed.' }
    if ((Get-FileHash $download -Algorithm SHA256).Hash -ne $boostHash) { throw 'Boost archive checksum mismatch.' }
    Move-Item -LiteralPath $download -Destination $boost -Force
}

& $python -u (Join-Path $source 'build_scripts/build_usd.py') $install `
    --build-variant release --no-imaging --no-usdview --python --materialx `
    --generator 'Visual Studio 18 2026' --toolset v145 --jobs $Jobs `
    --build-args "boost,--user-config=$($boostConfig.Replace('\', '/')) architecture=x86 --reconfigure" 'zlib,-DCMAKE_POLICY_VERSION_MINIMUM=3.5' `
    'USD,-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBoost_COMPILER=-vc143 -DBoost_ARCHITECTURE=-x64' 'MaterialX,-DCMAKE_POLICY_VERSION_MINIMUM=3.5'
if ($LASTEXITCODE -ne 0) { throw "OpenUSD build failed; dependency logs are under $install." }
& "$PSScriptRoot/build_usd_gltf.ps1" -Jobs $Jobs
Write-Output "OpenUSD Release SDK: $install"
