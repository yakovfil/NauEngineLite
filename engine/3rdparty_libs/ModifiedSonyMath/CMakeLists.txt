cmake_minimum_required(VERSION 3.20)

set(TargetName vectormath)

project(${TargetName}
  VERSION 1.54
  HOMEPAGE_URL "https://github.com/ConfettiFX/The-Forge/tree/master/Common_3/Utilities/ThirdParty/OpenSource/ModifiedSonyMath"
  LANGUAGES CXX
  )

nau_collect_files(Sources
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  MASK "*.h" "*.hpp"
)

add_library(${TargetName} INTERFACE ${Sources})


target_include_directories(${TargetName} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>  
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/3rdPartyLibs/ModifiedSonyMath/include>
)

if (NAU_MATH_USE_DOUBLE_PRECISION)
  target_compile_definitions(${TargetName} INTERFACE MATH_USE_DOUBLE_PRECISION)
endif()

nau_collect_files(HEADERS
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  DIRECTORIES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  MASK "*.h" "*.hpp" "*.ipp"
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/3rdPartyLibs/ModifiedSonyMath
    FILES_MATCHING 
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
)

nau_install(${TargetName} 3rdPartyLibs)