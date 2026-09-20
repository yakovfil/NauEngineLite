set (TargetName ${NAU_FRAMEWORK_TARGET})

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH moduleRoot)

if(NAU_RUNTIME_PROFILE STREQUAL "minimal")
  set(Sources
    app/application_impl.cpp
    app/application_setup.cpp
    app/background_work_service.cpp
    app/global_properties_impl.cpp
    app/logging_service.cpp
    app/platform_window_service.cpp
    main_loop/main_loop_service.cpp
    main_loop/concurrent_execution_container.cpp)
  list(TRANSFORM Sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
else()
  nau_collect_files(Sources
  DIRECTORIES ${moduleRoot}/src
  RELATIVE ${moduleRoot}/src
  MASK "*.cpp" "*.h"
  EXCLUDE
    "(^|/)platform/.*"
  )
endif()

nau_collect_files(Headers
  DIRECTORIES ${moduleRoot}/include
  RELATIVE ${moduleRoot}
  MASK "*.cpp" "*.h"
  EXCLUDE
    "/platform/.*"
  PREPEND "../"
)

add_library(${TargetName} STATIC ${Sources} ${PublicHeaders})
if(EMSCRIPTEN)
  target_sources(${TargetName} PRIVATE platform/emscripten/browser_runtime.cpp)
endif()

target_precompile_headers(${TargetName} PRIVATE pch.h)

target_include_directories(${TargetName} PUBLIC
  $<BUILD_INTERFACE:${moduleRoot}/include>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/app_framework/include>
)

target_include_directories(${TargetName} PRIVATE
  $<BUILD_INTERFACE:${moduleRoot}/src>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/app_framework/src>
)

target_compile_definitions(${TargetName} PRIVATE
  NAU_FRAMEWORK_BUILD=1
)

target_link_libraries(${TargetName} PUBLIC NauKernel PlatformAppApi)
if(NAU_RUNTIME_PROFILE STREQUAL "minimal")
  target_compile_definitions(${TargetName} PRIVATE NAU_MINIMAL_RUNTIME=1)
else()
  target_link_libraries(${TargetName} PUBLIC CoreScene Graphics ui VFX)
endif()

nau_add_compile_options(${TargetName})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX Sources FILES ${Sources})
source_group(TREE ${moduleRoot} PREFIX Headers FILES ${Headers})

nau_install(${TargetName} core)

install(DIRECTORY ${moduleRoot}/include
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/app_framework
    FILES_MATCHING 
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
)
