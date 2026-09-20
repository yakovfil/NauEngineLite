cmake_minimum_required(VERSION 3.24)
include(../../NauEmscriptenSdk.cmake)
set(EMSCRIPTEN_SYSTEM_PROCESSOR wasm32 CACHE STRING "Browser target processor")
project(NauEmscriptenProbe LANGUAGES C CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This toolchain probe requires the external Emscripten CMake toolchain.")
endif()

get_filename_component(engine "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${engine}/cmake")
include(NauCommon)
include(platformSpec/NauPlatformSetup)

# Exercise the production helper on bundled sources. This is not a declaration
# of the future minimal profile. The optional fmt check covers its C++20
# consteval regression separately from the smaller SDK capability probe.
set(BUILD_SHARED_LIBS OFF)
add_subdirectory("${engine}/engine/3rdparty_libs/xxHash" xxHash)
nau_target_add_compile_options(TARGETS xxHash)

add_executable(nau_emscripten_probe probe.cpp)
target_link_libraries(nau_emscripten_probe PRIVATE xxHash)
nau_add_compile_options(nau_emscripten_probe STRICT)
target_link_options(nau_emscripten_probe PRIVATE -sEXIT_RUNTIME=1)

option(NAU_PROBE_FMT "Also check the recorded bundled fmt sources" OFF)
if(NAU_PROBE_FMT)
    set(FMT_TEST OFF CACHE BOOL "" FORCE)
    set(FMT_DOC OFF CACHE BOOL "" FORCE)
    add_subdirectory("${engine}/engine/3rdparty_libs/fmt" fmt)
    nau_target_add_compile_options(TARGETS fmt)
    target_link_libraries(nau_emscripten_probe PRIVATE fmt)
    target_compile_definitions(nau_emscripten_probe PRIVATE NAU_PROBE_FMT)
endif()

file(WRITE "${CMAKE_BINARY_DIR}/platform.txt"
    "target=${Target_Arch}\nhost=${Host_Arch}\nwindows=${Platform_Windows}\nwin32=${Platform_Win32}\nwin64=${Platform_Win64}\nemscripten=${Platform_Emscripten}\n")
