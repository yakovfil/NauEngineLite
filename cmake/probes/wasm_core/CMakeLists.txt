cmake_minimum_required(VERSION 3.24)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/.cmake/api/v1/query")
file(WRITE "${CMAKE_BINARY_DIR}/.cmake/api/v1/query/codemodel-v2" "")
get_filename_component(engine "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
if(CMAKE_TOOLCHAIN_FILE MATCHES "Emscripten\\.cmake$" OR CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
  include("${engine}/cmake/NauEmscriptenSdk.cmake")
  if(NOT EXISTS "$ENV{EMSDK_NODE}")
    message(FATAL_ERROR "EMSDK_NODE is missing; dot-source the active SDK emsdk_env.ps1 before core validation.")
  endif()
  set(CMAKE_CROSSCOMPILING_EMULATOR "$ENV{EMSDK_NODE}" CACHE STRING "Use the activated SDK Node runtime" FORCE)
  set(EMSCRIPTEN_SYSTEM_PROCESSOR wasm32 CACHE STRING "Target architecture")
endif()
project(NauCorePortability LANGUAGES C CXX)
list(APPEND CMAKE_MODULE_PATH "${engine}/cmake")
include(NauCommon)
include(NauModule)
include(platformSpec/NauPlatformSetup)
set(NAU_RUNTIME_PROFILE minimal)
set(NAU_MINIMAL_TESTS ON)
include(NauMinimalRuntime)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(NauEngineFolder NauCorePortability)
include(NauGitVersion)
git_check_version()
file(STRINGS "${engine}/.nauversion" NAU_VERSION_STRING)
nau_set_version("${NAU_VERSION_STRING}")
enable_testing()
set(NAU_MINIMAL_GMOCK ON)
add_subdirectory("${engine}/engine/3rdparty_libs" dependencies)
add_subdirectory("${engine}/engine/core/kernel/src" kernel)
# Header contract only; no PlatformApp implementation or browser application is linked.
add_library(PlatformAppApi INTERFACE)
target_include_directories(PlatformAppApi INTERFACE "$<BUILD_INTERFACE:${engine}/engine/core/modules/platform_app/include>")
target_compile_definitions(PlatformAppApi INTERFACE NAU_PLATFORMAPP_EXPORT=)
nau_install(PlatformAppApi core)
add_subdirectory("${engine}/engine/core/app_framework/src" framework)
add_executable(NauCorePortabilityTests "${engine}/engine/core/kernel/tests/test_wasm_core/test_core.cpp")
nau_write_static_modules_initialization(moduleRegistration CoreTest)
target_sources(NauCorePortabilityTests PRIVATE "${moduleRegistration}")
target_compile_definitions(NauCorePortabilityTests PRIVATE NAU_MODULE_NAME=CoreTest)
target_include_directories(NauCorePortabilityTests PRIVATE "${engine}/engine/core/app_framework/src")
target_sources(NauCorePortabilityTests PRIVATE
  "${engine}/engine/core/kernel/tests/test_runtime/test_cancellation_timeout.cpp"
  "${engine}/engine/core/kernel/tests/test_wasm_core/test_transform_portability.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_serialization/test_math_serialization.cpp")
target_sources(NauCorePortabilityTests PRIVATE
  "${engine}/engine/core/kernel/tests/test_wasm_core/test_std_migration.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_serialization/test_json.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_serialization/test_json_mutability.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_serialization/test_runtime_value.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_serialization/test_runtime_value_events.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_enum_reflection.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_type_info.cpp"
  "${engine}/engine/core/kernel/tests/test_runtime/test_uid.cpp")
target_precompile_headers(NauCorePortabilityTests PRIVATE
  "${engine}/engine/core/kernel/tests/test_runtime/pch.h")
target_link_libraries(NauCorePortabilityTests PRIVATE NauKernel NauFrameworkMinimal gtest_main gmock)
nau_add_compile_options(NauCorePortabilityTests)
if(EMSCRIPTEN)
  target_link_options(NauCorePortabilityTests PRIVATE -sEXIT_RUNTIME=1)
  add_test(NAME CorePortability COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:NauCorePortabilityTests>)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  add_test(NAME CoreFatalDiagnostic COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_failure.py"
    ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:NauCorePortabilityTests>
    --gtest_filter=CorePortability.DISABLED_FatalDiagnostic --gtest_also_run_disabled_tests)
  set_tests_properties(CoreFatalDiagnostic PROPERTIES TIMEOUT 40)
else()
  add_test(NAME CorePortability COMMAND NauCorePortabilityTests)
endif()
set_tests_properties(CorePortability PROPERTIES TIMEOUT 60)

# Exercise native kernel ownership and worker lifetimes without desktop module dependencies.
if(NOT EMSCRIPTEN)
  set(runtimeTests "${engine}/engine/core/kernel/tests/test_runtime")
  set(commonTests "${engine}/engine/core/kernel/tests/test_common")
  add_executable(NauKernelMigrationTests
    "${runtimeTests}/helpers/runtime_guard.cpp"
    "${commonTests}/stopwatch.cpp"
    "${commonTests}/assert_catcher_guard.cpp"
    "${runtimeTests}/test_cancellation.cpp"
    "${runtimeTests}/test_standard_allocator.cpp"
    "${runtimeTests}/test_allocators.cpp"
    "${runtimeTests}/test_diag/test_asserts.cpp"
    "${runtimeTests}/test_async/test_async_executor.cpp"
    "${runtimeTests}/test_async/test_async_functions.cpp"
    "${runtimeTests}/test_async/test_multi_task_source.cpp"
    "${runtimeTests}/test_async/test_task.cpp"
    "${runtimeTests}/test_async/test_task_collection.cpp"
    "${runtimeTests}/test_async/test_thread_local_value.cpp"
    "${engine}/engine/core/kernel/tests/test_kernel/test_service.cpp"
    "${engine}/engine/core/kernel/tests/test_kernel/test_io/test_fs_path.cpp")
  target_include_directories(NauKernelMigrationTests PRIVATE "${runtimeTests}" "${commonTests}/include")
  target_precompile_headers(NauKernelMigrationTests PRIVATE "${runtimeTests}/pch.h")
  target_link_libraries(NauKernelMigrationTests PRIVATE NauKernel gtest_main gmock)
  nau_add_compile_options(NauKernelMigrationTests)
  include(GoogleTest)
  gtest_discover_tests(NauKernelMigrationTests TEST_PREFIX KernelMigration.
    DISCOVERY_TIMEOUT 30 PROPERTIES TIMEOUT 30)
endif()

# Header-level renderer contracts, independent of a GPU or desktop driver.
add_executable(NauRendererContainerTests "${engine}/engine/core/modules/render/tests/test_render/test_std_containers.cpp")
target_include_directories(NauRendererContainerTests PRIVATE "${engine}/engine/core/modules/render/include")
target_link_libraries(NauRendererContainerTests PRIVATE NauKernel gtest_main)
nau_add_compile_options(NauRendererContainerTests)
if(EMSCRIPTEN)
  target_link_options(NauRendererContainerTests PRIVATE -sEXIT_RUNTIME=1)
endif()
add_test(NAME RendererContainers COMMAND ${CMAKE_CROSSCOMPILING_EMULATOR} $<TARGET_FILE:NauRendererContainerTests>)
set_tests_properties(RendererContainers PROPERTIES TIMEOUT 30)
