set(TargetName CoreAssets)

nau_collect_files(Sources
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/src
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  
  MASK "*.cpp" "*.h"
)

nau_collect_files(PublicHeaders
  DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/include
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  
  MASK "*.h"
)

if(NAU_RUNTIME_PROFILE STREQUAL "demo")
  set(Sources ${NAU_DEMO_CoreAssets_SOURCES})
endif()

nau_add_module(${TargetName}
  SOURCES ${Sources} ${PublicHeaders}
  FOLDER "${NauEngineFolder}/core/modules"
  EXPORT_AS_LIBRARY
)

target_compile_definitions(${TargetName} PRIVATE NAU_COREASSETS_BUILD)

target_precompile_headers(${TargetName} PRIVATE src/pch.h)

target_include_directories(${TargetName} PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/assets/src>
)

target_include_directories(${TargetName} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/assets/include>
)

target_link_libraries(${TargetName} PUBLIC tinyimageformat)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX Source FILES ${Sources})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX Headers FILES ${PublicHeaders})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/modules/assets
    FILES_MATCHING 
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
)

nau_install(${TargetName} core)