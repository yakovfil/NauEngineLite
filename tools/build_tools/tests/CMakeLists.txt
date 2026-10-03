set(TargetName "BuildToolTests")

nau_collect_files(Sources
ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
  MASK "*.cpp"
)

add_executable(${TargetName} ${Sources})
add_test(NAME BuildTools COMMAND ${TargetName})

set_target_properties(
  ${TargetName}
  PROPERTIES
  FOLDER "${NauEngineFolder}/tests"
)

target_include_directories(${TargetName} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TargetName} PRIVATE
  NauKernel
  BuildTool
  Shared
  nlohmann_json::nlohmann_json
  gtest
  gmock
)

nau_add_compile_options(${TargetName})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})
include(GoogleTest)
gtest_discover_tests(${TargetName})