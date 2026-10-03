add_subdirectory(src)

if(NAU_CORE_TESTS)
  add_subdirectory(tests/test_common)
  add_subdirectory(tests/test_kernel)
  add_subdirectory(tests/test_runtime)
endif()
