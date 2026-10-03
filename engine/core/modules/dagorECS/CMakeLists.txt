
add_subdirectory(src)

# TODO NAU-2089
# if (NAU_CORE_TESTS)
#     nau_collect_cmake_subdirectories(tests ${CMAKE_CURRENT_SOURCE_DIR}/tests)
#     foreach(test ${tests})
#         add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests/${test})
#     endforeach()
# endif()
if(NAU_CORE_TESTS)
    add_executable(test_ecs_std tests/test_std_ecs.cpp)
    target_link_libraries(test_ecs_std PRIVATE gtest DagorECS NauFramework)
    nau_target_link_modules(test_ecs_std)
    nau_add_compile_options(test_ecs_std)
    add_test(NAME EcsStandardContainers COMMAND test_ecs_std)
    set_tests_properties(EcsStandardContainers PROPERTIES TIMEOUT 45)
endif()
