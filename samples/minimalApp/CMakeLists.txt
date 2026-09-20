set(TargetName MinimalAppSample)

if(NAU_RUNTIME_PROFILE STREQUAL "minimal")
  set(Sources "${CMAKE_CURRENT_SOURCE_DIR}/app_sample_main_min.cpp")
  set(Modules ${NAU_MINIMAL_MODULES})
else()
  set(Sources "${CMAKE_CURRENT_SOURCE_DIR}/app_sample_main.cpp")
  set(Modules PlatformApp)
endif()

add_executable(${TargetName} ${Sources})

target_link_libraries(${TargetName} PRIVATE
  ${NAU_FRAMEWORK_TARGET}
  PlatformAppApi
)

target_precompile_headers(${TargetName} PRIVATE pch.h)

nau_target_link_modules(${TargetName}
  ${Modules}
)

nau_add_compile_options(${TargetName})
if(EMSCRIPTEN)
  target_link_options(${TargetName} PRIVATE
    -sEXIT_RUNTIME=1 -sPTHREAD_POOL_SIZE=12 -sALLOW_BLOCKING_ON_MAIN_THREAD=0)
  add_custom_target(MinimalAppWebPackage ALL
    COMMAND ${CMAKE_COMMAND}
      "-DBUILD_ROOT=${CMAKE_BINARY_DIR}"
      "-DDESTINATION=${CMAKE_BINARY_DIR}/package"
      "-DLOADER=$<TARGET_FILE:${TargetName}>"
      "-DWASM=$<TARGET_FILE_DIR:${TargetName}>/MinimalAppSample.wasm"
      "-DASSETS=${CMAKE_CURRENT_SOURCE_DIR}/web"
      "-DCONFIGURATION=$<CONFIG>"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/web/stage.cmake"
    DEPENDS ${TargetName}
    VERBATIM)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})

set_target_properties (${TargetName} PROPERTIES
    FOLDER "${NauEngineFolder}/samples"
)
