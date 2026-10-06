cmake_minimum_required(VERSION 3.24)
project(NauUsdGltf LANGUAGES C CXX)

if(NOT NAU_USD_SDK_DIR)
    message(FATAL_ERROR "Set NAU_USD_SDK_DIR to the locally built OpenUSD SDK")
endif()
file(TO_CMAKE_PATH "${NAU_USD_SDK_DIR}" NAU_USD_SDK_DIR)
set(pxr_ROOT "${NAU_USD_SDK_DIR}")
set(pxr_DIR "${NAU_USD_SDK_DIR}")
find_package(pxr REQUIRED)
find_package(OpenImageIO CONFIG REQUIRED)
# OpenUSD 24.08 is built with Boost 1.78 by build_openusd.ps1.
set(USD_BOOST_VERSION 107800)
# USD's public headers must use the same Boost headers as the SDK, ahead of
# the newer transitive headers provided by OpenImageIO's vcpkg dependencies.
include_directories(BEFORE "${NAU_USD_SDK_DIR}/include/boost-1_78")
add_compile_definitions(BOOST_ALL_NO_LIB)
include(FetchContent)
# Pin the upstream TinyGLTF 2.8.21 tag's revision. GitHub's generated release
# archives no longer match the hashes in the corresponding vcpkg ports.
FetchContent_Declare(TinyGLTF
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG d9c2bbe90c6af6fca5e2d3f8f8e0091e0a913e48)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)
set(TINYGLTF_HEADER_ONLY ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(TinyGLTF)
add_library(tinygltf::tinygltf ALIAS tinygltf)
set(USD_FILEFORMATS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
foreach(format FBX OBJ PLY STL SBSAR DRACO)
    set(USD_FILEFORMATS_ENABLE_${format} OFF CACHE BOOL "" FORCE)
endforeach()
set(USD_FILEFORMATS_ENABLE_GLTF ON CACHE BOOL "" FORCE)
set(USD_FILEFORMATS_FETCH_TINYGLTF OFF CACHE BOOL "" FORCE)
set(USD_FILEFORMATS_FETCH_GTEST OFF CACHE BOOL "" FORCE)
set(USD_FILEFORMATS_FETCH_ZLIB OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../engine/3rdparty_libs/USD-Fileformat-plugins" plugins)

# Third-party dependencies use static libraries with the dynamic CRT. Their
# DLL names (notably fmt.dll) would otherwise collide with engine dependencies.
