@echo Build mode sample usage

cd ../..

if exist output (
    rmdir /S /Q output
) 

del *.out 

rem set preset=win_vs2026_x64
set preset=win_vs2026_x64_dll

set config=Debug
rem set config=Release

cmake --preset %preset% --graphviz=build\%preset%.dot 1>cmake_log.out 2>cmake_err.out
if errorlevel 1 exit /B

cmake --build build\%preset% --parallel --config=%config% 1>build_log.out 2>build_err.out
if errorlevel 1 exit /B

cmake --install build\%preset% --config=%config% 1>install_log.out 2>install_err.out
if errorlevel 1 exit /B

cd output
>NUL SETX NAU_ENGINE_SDK_DIR %CD%

cmake --preset %preset% 1>cmake_log.out 2>cmake_err.out
if errorlevel 1 exit /B

cmake --build build\%preset% --config=%config% 1>build_log.out 2>build_err.out
if errorlevel 1 exit /B

@echo on

@rem print variables
@echo NAU_ENGINE_SDK_DIR = %CD%