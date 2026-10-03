cmake_minimum_required(VERSION 3.16)

project(binPack2D
    LANGUAGES CXX
    VERSION 1.0
)

set(TargetName binPack2D)


nau_collect_files(Sources
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  MASK "*.cpp" "*.h" "*.hpp"
)

nau_collect_files(Headers
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  MASK "*.h" "*.hpp"
)

add_library(${TargetName} ${Sources} ${Headers})

target_link_libraries(${TargetName}
  NauKernel
)

target_include_directories(${TargetName} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(${TargetName} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/3rdPartyLibs/binPack2D/include>
)

if (BUILD_SHARED_LIBS)
  target_compile_definitions(${TargetName} PRIVATE
    NAU_BINPACK_BUILD
  )
endif()

nau_collect_files(HEADERS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  MASK "*.h" "*.hpp" "*.ipp"
)

nau_install(${TargetName} 3rdPartyLibs ${HEADERS})
