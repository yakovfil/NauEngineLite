# Small kernel dependency graph shared with optional minimal runtime consumers.
set(NAU_MINIMAL_DEPENDENCY_DIRS
  fmt utfcpp ModifiedSonyMath jsoncpp wyhash fast_float)
set(NAU_MINIMAL_DEPENDENCY_TARGETS
  fmt utf8cpp vectormath jsoncpp wyhash fast_float)
foreach(dependency IN LISTS NAU_MINIMAL_DEPENDENCY_DIRS)
  if(NOT EXISTS "${NAU_ENGINE_SOURCE_DIR}/engine/3rdparty_libs/${dependency}/CMakeLists.txt")
    message(FATAL_ERROR "Missing minimal runtime dependency: ${dependency}. Restore recorded bundled sources with git submodule update --init --recursive.")
  endif()
endforeach()

set(BUILD_SHARED_LIBS OFF)
set(NAU_ENGINE_KERNEL ON)
set(NAU_CORE_TOOLS OFF)
set(NAU_FORCE_ENABLE_SHADER_COMPILER_TOOL OFF)
set(NAU_CORE_TESTS OFF)
option(NAU_MINIMAL_TESTS "Build focused native minimal lifecycle tests when attached" OFF)
if(NAU_MINIMAL_TESTS AND NOT EXISTS "${NAU_ENGINE_SOURCE_DIR}/engine/3rdparty_libs/googletest/CMakeLists.txt")
  message(FATAL_ERROR "Missing minimal runtime test dependency: googletest. Restore recorded bundled sources with git submodule update --init --recursive.")
endif()
