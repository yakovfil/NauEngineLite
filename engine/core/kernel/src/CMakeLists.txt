set(TargetName NauKernel)

cmake_path(GET CMAKE_CURRENT_SOURCE_DIR PARENT_PATH moduleRoot)

if(NOT NAU_RUNTIME_PROFILE MATCHES "^(minimal|demo)$")
nau_collect_files(Sources
  DIRECTORIES ${moduleRoot}/src
  RELATIVE ${moduleRoot}/src
  MASK "*.cpp" "*.h" "*.hpp"
  EXCLUDE "(^|/)platform/.*"
)

endif()

nau_collect_files(PublicHeaders
  DIRECTORIES ${moduleRoot}/include
  RELATIVE ${moduleRoot}
  MASK "*.cpp" "*.h" "*.hpp"
  EXCLUDE "/platform/.*"
  PREPEND "../"
)


if (Platform_Windows AND NOT NAU_RUNTIME_PROFILE MATCHES "^(minimal|demo)$")

  nau_collect_files(Sources
    DIRECTORIES ${moduleRoot}/src
    RELATIVE ${moduleRoot}/src
    INCLUDE
      "(^|/)platform/windows/.*"
    MASK "*.cpp" "*.h" "*.hpp"
  )

  nau_collect_files(PublicHeaders
    DIRECTORIES ${moduleRoot}/include/
    RELATIVE ${moduleRoot}
    INCLUDE
    "/platform/windows/.*"
    MASK "*.h" "*.hpp"
    PREPEND "../"
  )
endif()


if(NAU_RUNTIME_PROFILE MATCHES "^(minimal|demo)$")
  include(NauMinimalKernelSources)
  set(Sources ${NAU_MINIMAL_KERNEL_COMMON_SOURCES})
  if(Platform_Windows)
    list(APPEND Sources ${NAU_MINIMAL_KERNEL_WINDOWS_SOURCES})
  else()
    list(APPEND Sources ${NAU_MINIMAL_KERNEL_EMSCRIPTEN_SOURCES})
  endif()
  list(TRANSFORM Sources PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
endif()

add_library(${TargetName} ${Sources} ${PublicHeaders})

if (Compiler_Cl)
  target_sources(${TargetName} PRIVATE
    ${moduleRoot}/extras/Visualizers/nau_kernel.natjmc
    ${moduleRoot}/extras/Visualizers/nau_kernel.natvis
    ${moduleRoot}/extras/Visualizers/third_party_libs.natjmc
  )
endif()

#target_sources(${TargetName} PRIVATE ${Headers})

target_precompile_headers(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/pch.h)

if (NAU_VERBOSE_LOG)
  target_compile_definitions(${TargetName} PUBLIC
      NAU_VERBOSE_LOG=1
  )
endif()
if (NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${TargetName} PUBLIC
    NAU_STATIC_RUNTIME=1
  )
else()
  target_compile_definitions(${TargetName} PUBLIC  
    NAU_RUNTIME_BUILD=1
  )
endif()

target_compile_definitions(${TargetName} PRIVATE
  NAU_KERNEL_BUILD=1
)

if(Platform_Windows)
  target_compile_definitions(${TargetName} PRIVATE _TARGET_PC_WIN=1)
endif()

target_include_directories(${TargetName} PUBLIC
  $<BUILD_INTERFACE:${moduleRoot}/include>
  $<BUILD_INTERFACE:${PROG_DIR}/dagorInclude>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/kernel/include>
)

if(NAU_RUNTIME_PROFILE MATCHES "^(minimal|demo)$")
  target_link_libraries(${TargetName} PUBLIC ${NAU_MINIMAL_DEPENDENCY_TARGETS})
else()
target_link_libraries(${TargetName} PUBLIC
  EASTL
  tinyutf8
  fmt
  utf8cpp
  vectormath
  jsoncpp
  wyhash
  brotli
  lzma-9.20
  zlib-ng
  zstd-1.4.5
  md5
  fast_float
)
endif()

if (Platform_Windows)
  # Rpcrt4.lib required by uid implmentaion
  if (CMAKE_BUILD_SHARED_LIBS)
    target_link_libraries(${TargetName} PUBLIC Rpcrt4.lib version.lib)
  else()
    target_link_libraries(${TargetName} PRIVATE Rpcrt4.lib version.lib)
  endif()
endif()

if (Compiler_MSVC)
  target_compile_options(${TargetName} PRIVATE
#    /Zc:preprocessor
    /GR-
  )
elseif(Compiler_ClangCl)
  target_compile_options(${TargetName} PRIVATE 
    /clang:-fno-rtti)

endif()

nau_add_compile_options(${TargetName})
#add_nau_folder_property(${TargetName} LIB)
source_group(TREE ${moduleRoot}/src PREFIX Sources FILES ${Sources})
source_group(TREE ${moduleRoot}/include PREFIX Headers FILES ${PublicHeaders})

nau_install(${TargetName} core)

install(DIRECTORY ${moduleRoot}/include
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/kernel
    FILES_MATCHING 
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
)
