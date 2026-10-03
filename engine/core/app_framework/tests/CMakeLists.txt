add_executable(NauFrameworkCoreTests
  main.cpp
  test_global_properties.cpp
  test_browser_runtime_timing.cpp
  "${CMAKE_CURRENT_SOURCE_DIR}/../src/app/global_properties_impl.cpp")

target_include_directories(NauFrameworkCoreTests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/../include"
  "${CMAKE_CURRENT_SOURCE_DIR}/../src")
target_link_libraries(NauFrameworkCoreTests PRIVATE NauKernel gtest)
target_precompile_headers(NauFrameworkCoreTests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src/pch.h")

nau_add_compile_options(NauFrameworkCoreTests)
set_target_properties(NauFrameworkCoreTests PROPERTIES FOLDER "${NauEngineFolder}/tests")

include(GoogleTest)
gtest_discover_tests(NauFrameworkCoreTests DISCOVERY_TIMEOUT 10)
