# Validate target compilers, never the Windows host of an Emscripten build.
if(EMSCRIPTEN OR NOT WIN32)
    return()
endif()

set(_nau_toolchain_help "Nau requires Visual Studio 2026 C++ tools (v145), x64 and CMake 4.2 or newer. Configure a clean build directory using win_vs2026_x64 (or its DLL variant); do not reuse a VS 2022 cache.")

if(CMAKE_VERSION VERSION_LESS "4.2")
    message(FATAL_ERROR "${_nau_toolchain_help} Detected CMake ${CMAKE_VERSION}.")
endif()

if(CMAKE_GENERATOR MATCHES "^Visual Studio" AND
   NOT CMAKE_GENERATOR STREQUAL "Visual Studio 18 2026")
    message(FATAL_ERROR "${_nau_toolchain_help} Detected generator '${CMAKE_GENERATOR}'.")
endif()

# MSVC is also true for clang-cl: only cl.exe uses the MSVC version mapping.
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(NOT DEFINED MSVC_VERSION OR MSVC_VERSION LESS 1950 OR MSVC_VERSION GREATER_EQUAL 1960)
        message(FATAL_ERROR "${_nau_toolchain_help} Detected MSVC ${CMAKE_CXX_COMPILER_VERSION} (${MSVC_VERSION}).")
    endif()
    if(CMAKE_VS_PLATFORM_TOOLSET AND NOT CMAKE_VS_PLATFORM_TOOLSET STREQUAL "v145")
        message(FATAL_ERROR "${_nau_toolchain_help} Detected toolset '${CMAKE_VS_PLATFORM_TOOLSET}'.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR
       (CMAKE_GENERATOR_PLATFORM AND NOT CMAKE_GENERATOR_PLATFORM MATCHES "^x64(,|$)") OR
       (CMAKE_CXX_COMPILER_ARCHITECTURE_ID AND NOT CMAKE_CXX_COMPILER_ARCHITECTURE_ID STREQUAL "x64"))
        message(FATAL_ERROR "${_nau_toolchain_help} Detected architecture '${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}/${CMAKE_GENERATOR_PLATFORM}'.")
    endif()
endif()

message(STATUS "Nau Windows toolchain: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; generator=${CMAKE_GENERATOR}; toolset=${CMAKE_VS_PLATFORM_TOOLSET}; SDK=${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
unset(_nau_toolchain_help)
