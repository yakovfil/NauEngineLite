# Authoritative source graph for the non-rendering runtime.
set(NAU_MINIMAL_MODULES "PlatformApp" CACHE STRING "Required minimal runtime modules")
if(NOT NAU_MINIMAL_MODULES STREQUAL "PlatformApp")
  message(FATAL_ERROR "Unsupported minimal modules '${NAU_MINIMAL_MODULES}'. Supported and required set: PlatformApp.")
endif()
set(NAU_MINIMAL_MODULE_DIRS platform_app)
set(NAU_MINIMAL_DEPENDENCY_DIRS
  EABase EASTL tiny-utf8 fmt utfcpp ModifiedSonyMath jsoncpp wyhash fast_float)
set(NAU_MINIMAL_DEPENDENCY_TARGETS
  EABase EASTL tinyutf8 fmt utf8cpp vectormath jsoncpp wyhash fast_float)
foreach(dependency IN LISTS NAU_MINIMAL_DEPENDENCY_DIRS)
  if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../engine/3rdparty_libs/${dependency}/CMakeLists.txt")
    message(FATAL_ERROR "Missing minimal runtime dependency: ${dependency}. Restore recorded bundled sources with git submodule update --init --recursive.")
  endif()
endforeach()

# These settings select the profile without changing desktop cache defaults.
set(BUILD_SHARED_LIBS OFF)
set(NAU_ENGINE_KERNEL ON)
set(NAU_ENGINE_MODULES ON)
set(NAU_ENGINE_FRAMEWORK ON)
set(NAU_CORE_TOOLS OFF)
set(NAU_FORCE_ENABLE_SHADER_COMPILER_TOOL OFF)
set(NAU_CORE_TESTS OFF)
option(NAU_MINIMAL_TESTS "Build focused native minimal lifecycle tests" OFF)
if(NAU_MINIMAL_TESTS AND NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/../engine/3rdparty_libs/googletest/CMakeLists.txt")
  message(FATAL_ERROR "Missing minimal runtime test dependency: googletest. Restore recorded bundled sources with git submodule update --init --recursive.")
endif()
set(NAU_FRAMEWORK_TARGET NauFrameworkMinimal)
