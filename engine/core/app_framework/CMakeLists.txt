add_subdirectory(src)

if(NAU_RUNTIME_PROFILE STREQUAL "minimal" AND NAU_MINIMAL_TESTS)
    add_subdirectory(tests/test_minimal_app)
endif()

if (NAU_CORE_TESTS)
    nau_collect_cmake_subdirectories(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    foreach(test ${tests})
        add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests/${test})
    endforeach()
endif()
