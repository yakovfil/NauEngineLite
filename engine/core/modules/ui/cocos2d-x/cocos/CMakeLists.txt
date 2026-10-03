#/****************************************************************************
# Copyright (c) 2013 cocos2d-x.org
# Copyright (c) 2014 martell malone
# Copyright (c) 2015-2017 Chukong Technologies Inc.
#
# http://www.cocos2d-x.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# ****************************************************************************/

# this CMakeLists is to generate Cocos2d-x Engine Library
# build libcocos2d

# The version number
set(COCOS2D_X_VERSION 4.0)

set(COCOS_CORE_LIB cocos2d)
project(cocos2d_libs)

set(CMAKE_CXX_STANDARD 20)

include(2d/CMakeLists.txt)
include(platform/CMakeLists.txt)
include(math/CMakeLists.txt)
include(renderer/CMakeLists.txt)
include(base/CMakeLists.txt)

# default value for cocos2dx extensions modules build
option(BUILD_EXTENSIONS "Build extension library" ON)
option(BUILD_COCOS_WITH_OPENGL "Build cocos with openGL library" OFF)
option(BUILD_COCOS_WITH_DAGOR "Build cocos with dagor-render library" ON)

if(BUILD_EXTENSIONS)
    include(../extensions/CMakeLists.txt)
endif(BUILD_EXTENSIONS)

set(COCOS_HEADER cocos2d.h
    ${COCOS_2D_HEADER}
    ${COCOS_PLATFORM_HEADER}
    ${COCOS_PHYSICS_HEADER}
    ${COCOS_PHYSICS3D_HEADER}
    ${COCOS_MATH_HEADER}
    ${COCOS_NAVMESH_HEADER}
    ${COCOS_RENDERER_HEADER}
    ${COCOS_BASE_HEADER}
    ${COCOS_AUDIO_HEADER}
    ${COCOS_NETWORK_HEADER}
    ${COCOS_EDITOR_SUPPORT_HEADER}
    ${COCOS_EXTENSIONS_HEADER}
    )

set(COCOS_SRC cocos2d.cpp
    ${COCOS_2D_SRC}
    ${COCOS_PLATFORM_SRC}
    ${COCOS_PHYSICS_SRC}
    ${COCOS_PHYSICS3D_SRC}
    ${COCOS_MATH_SRC}
    ${COCOS_NAVMESH_SRC}
    ${COCOS_RENDERER_SRC}
    ${COCOS_BASE_SRC}
    ${COCOS_AUDIO_SRC}
    ${COCOS_NETWORK_SRC}
    ${COCOS_EDITOR_SUPPORT_SRC}
    ${COCOS_EXTENSIONS_SRC}
    ${COCOS_STORAGE_SRC}
    )

list(APPEND COCOS_SRC ${COCOS_HEADER})

add_library(cocos2d ${COCOS_SRC})

if(BUILD_COCOS_WITH_OPENGL)
    target_compile_definitions(cocos2d PRIVATE BUILD_COCOS_WITH_OPENGL=1)
endif()

if(BUILD_COCOS_WITH_DAGOR)
    target_compile_definitions(cocos2d PUBLIC BUILD_COCOS_WITH_DAGOR=1)
endif()

# use external libs
add_subdirectory(${COCOS2DX_ROOT_PATH}/external ${ENGINE_BINARY_PATH}/external)
target_link_libraries(cocos2d
    PUBLIC
        external
        NauKernel
        Render
)

# add base macro define and compile options
use_cocos2dx_compile_define(cocos2d)
use_cocos2dx_compile_options(cocos2d)

# use all platform related system libs
use_cocos2dx_libs_depend(cocos2d)

target_include_directories(cocos2d PUBLIC
  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x>
  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/cocos>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/cocos>
  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/extensions>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/extensions>
  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/cocos/platform>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/cocos/platform>
#  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/cocos/editor-support>
#  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/cocos/editor-support>
)

target_include_directories(cocos2d INTERFACE
  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/cocos/base>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/cocos/base>
#  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/cocos/audio/include>
#  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/cocos/audio/include>
  $<BUILD_INTERFACE:${COCOS2DX_ROOT_PATH}/cocos/platform/${PLATFORM_FOLDER}>
  $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/core/modules/ui/cocos2d-x/cocos/platform/${PLATFORM_FOLDER}>
)

set_target_properties(cocos2d
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                      LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
                      VERSION "${COCOS2D_X_VERSION}"
                      FOLDER "${NauEngineFolder}/3rdparty"
                      )

nau_add_compile_options(cocos2d ENABLE_RTTI)

if(XCODE OR VS)
    cocos_mark_code_files("cocos2d")
endif()

if(${CMAKE_VERSION} VERSION_GREATER "3.16.0")
    message("CMake 3.16 target_precompile_headers")
    target_precompile_headers(cocos2d PRIVATE 
       "$<$<COMPILE_LANGUAGE:CXX>:cocos2d.h>")
endif()

#install(TARGETS cocos2d 
#  EXPORT cocos2d
#)

#export(EXPORT cocos2d
#  FILE ${CMAKE_BINARY_DIR}/cmake/NauCocos2dTargets.cmake
#)


#if(XCODE)
#    # Later versions of Xcode clang want to compile C++17 with aligned allocation turned on and this is only supported on iOS 11.0+
#    # TODO: Only turn this off if ${CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET} < 11.0
#    target_compile_options(cocos2d PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-aligned-allocation>)
#endif()

##!!TODO - review include path's
install(DIRECTORY ${COCOS2DX_ROOT_PATH}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/core/modules/ui
    FILES_MATCHING 
      PATTERN "*.h"
      PATTERN "*.hpp"
      PATTERN "*.ipp"
      PATTERN "*.inl"
)

nau_install(cocos2d core)
