
set(TargetName PlatformApp)
set(ApiTargetName ${TargetName}Api)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH moduleRoot)

nau_collect_files(Sources
  RELATIVE ${moduleRoot}/src
  DIRECTORIES ${moduleRoot}/src
  MASK "*.cpp" "*.h"
  EXCLUDE
    "(^|/)platform/.*"
)

if(Platform_Emscripten)
  nau_collect_files(Sources
    RELATIVE ${moduleRoot}/src
    DIRECTORIES ${moduleRoot}/src/platform/emscripten
    MASK "*.cpp" "*.h"
  )
elseif (${Platform_Windows})
  nau_collect_files(Sources
    RELATIVE ${moduleRoot}/src
    DIRECTORIES
      ${moduleRoot}/src/include/runtime/platform/windows
      ${moduleRoot}/src/platform/windows
    MASK "*.cpp" "*.h"
  )
endif()

nau_add_module(${TargetName}
  SOURCES ${Sources}
  FOLDER "${NauEngineFolder}/core/modules"
  INTERFACE_TARGET ${ApiTargetName}
)

target_precompile_headers(${TargetName} PRIVATE pch.h)
if(NAU_RUNTIME_PROFILE MATCHES "^(minimal|demo)$")
  target_compile_definitions(${TargetName} PRIVATE NAU_MINIMAL_RUNTIME=1)
endif()

target_include_directories(${TargetName} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/platform_app/src>
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})

## Module API
nau_collect_files(ApiHeaders
  DIRECTORIES ${moduleRoot}/include
  RELATIVE ${moduleRoot}
  MASK "*.cpp" "*.h"
  EXCLUDE
  "/platform/.*"
  PREPEND "../"
)

source_group(TREE ${moduleRoot}/include PREFIX Headers FILES ${ApiHeaders})

#target_sources(${ApiTargetName} INTERFACE ${ApiHeaders})

target_include_directories(${ApiTargetName} INTERFACE
  $<BUILD_INTERFACE:${moduleRoot}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/platform_app/include>
)

set_target_properties (${ApiTargetName} PROPERTIES
    FOLDER "${NauEngineFolder}/core/modules"
)

install(DIRECTORY ${moduleRoot}/include
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/modules/platform_app
    FILES_MATCHING 
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
)

nau_install(${ApiTargetName} core)
nau_install(${TargetName} core)
