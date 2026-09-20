
set(TargetName ScriptsLua)

nau_collect_files(Sources
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
)

nau_add_module(${TargetName}
  SOURCES ${Sources}
  FOLDER "${NauEngineFolder}/core/modules"
  EXPORT_AS_LIBRARY
)

target_compile_definitions(${TargetName} PRIVATE NAU_SCRIPTSLUA_BUILD)
target_precompile_headers(${TargetName} PRIVATE pch.h)

target_include_directories(${TargetName} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/scripts_lua/src>
)

# ScriptsLua exposes the kernel scripting interfaces and has no public header tree.

target_link_libraries(${TargetName} PRIVATE LuaToolkit)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})


nau_install(${TargetName} core)
