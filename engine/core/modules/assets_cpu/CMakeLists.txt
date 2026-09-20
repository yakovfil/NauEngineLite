if(NOT NAU_RUNTIME_PROFILE STREQUAL "demo")
  return()
endif()
set(TargetName CoreAssetsCpu)
set(assetFormats "${CMAKE_CURRENT_SOURCE_DIR}/../asset_formats/src")
nau_add_module(${TargetName}
  SOURCES ${NAU_DEMO_CoreAssetsCpu_SOURCES}
  FOLDER "${NauEngineFolder}/core/modules" EXPORT_AS_LIBRARY)
target_include_directories(${TargetName} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" PRIVATE "${assetFormats}")
target_link_libraries(${TargetName} PUBLIC CoreAssets)
target_precompile_headers(${TargetName} PRIVATE "${assetFormats}/pch.h")
