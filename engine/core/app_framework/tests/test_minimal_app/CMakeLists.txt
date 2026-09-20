if(NOT NAU_RUNTIME_PROFILE STREQUAL "minimal")
  return()
endif()

add_executable(NauMinimalLifecycleTests test_minimal_app.cpp test_service_failures.cpp test_application_failures.cpp)
target_sources(NauMinimalLifecycleTests PRIVATE test_browser_runtime_timing.cpp)
target_sources(NauMinimalLifecycleTests PRIVATE
  "../../../kernel/tests/test_kernel/test_service_dependencies.cpp"
  "../../../kernel/tests/test_kernel/test_service_init_proxy.cpp")
target_precompile_headers(NauMinimalLifecycleTests PRIVATE <gtest/gtest.h>)
target_link_libraries(NauMinimalLifecycleTests PRIVATE NauFrameworkMinimal PlatformAppApi gtest_main)
nau_target_link_modules(NauMinimalLifecycleTests ${NAU_MINIMAL_MODULES})
nau_add_compile_options(NauMinimalLifecycleTests)
add_test(NAME MinimalRuntime.Lifecycle COMMAND NauMinimalLifecycleTests)
set_tests_properties(MinimalRuntime.Lifecycle PROPERTIES TIMEOUT 30)
