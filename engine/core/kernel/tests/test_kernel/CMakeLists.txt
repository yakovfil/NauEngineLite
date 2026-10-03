
set(TargetName test_kernel)


nau_collect_files(SOURCES
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
  MASK "*.cpp" "*.h"
)


add_executable(${TargetName} ${SOURCES})
# TODO NAU-2089
# add_test(NAME Kernel COMMAND ${TargetName})

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
# Fix Ci
#include(GoogleTest)
#gtest_discover_tests(${TargetName} DISCOVERY_TIMEOUT 10)