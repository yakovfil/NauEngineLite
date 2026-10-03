add_library(TestAppSupport STATIC app_guard.cpp)

target_include_directories(TestAppSupport PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(TestAppSupport PRIVATE
  NAU_TEST_PROJECT_SOURCE_DIR="${NAU_TEST_PROJECT_SOURCE_DIR}")
target_link_libraries(TestAppSupport PUBLIC TestCommonLib ${NAU_FRAMEWORK_TARGET})

nau_add_compile_options(TestAppSupport)
set_target_properties(TestAppSupport PROPERTIES FOLDER "${NauEngineFolder}/tests")
