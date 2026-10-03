
set(TargetName test_runtime)


nau_collect_files(SOURCES
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
  # TODO NAU-2089
  EXCLUDE "test_string"
  MASK "*.cpp" "*.h"
)


add_executable(${TargetName} ${SOURCES})

target_precompile_headers(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)

target_include_directories(${TargetName} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TargetName} PRIVATE
  TestCommonLib
)

nau_add_compile_options(${TargetName})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
set_target_properties (${TargetName} PROPERTIES
    FOLDER "${NauEngineFolder}/tests"
)

include(GoogleTest)
gtest_discover_tests(${TargetName} DISCOVERY_TIMEOUT 10)