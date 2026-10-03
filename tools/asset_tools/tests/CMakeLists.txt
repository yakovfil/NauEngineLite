set(TargetName "test_asset_tools")

nau_collect_files(Sources
	ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR}
	DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}
	MASK "*.cpp"
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${Sources})
add_executable(${TargetName} ${Sources})
add_test(NAME AssetTools COMMAND ${TargetName})

# Metadata tests discover these plugins dynamically instead of linking them.
add_dependencies(${TargetName}
    NauUsdFormat NauComponentSchema NauInputSchema NauVFXSchema
    NauAudioSchema NauAnimationClipAsset NauGuiSchema NauPhysicsSchema NauAssetMetadata
)

set_target_properties(${TargetName}
	PROPERTIES
		FOLDER "${NauEngineFolder}/tests"
)

target_include_directories(${TargetName} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TargetName} PRIVATE
    NauKernel
    AssetTool
    Shared
    nlohmann_json::nlohmann_json
    gtest
    gmock

    usd
    UsdProxy
)

nau_add_compile_options(${TargetName})

if (WIN32)
    set(CONFIG_PLUGIN_PATH "$<$<CONFIG:DEBUG>:${CMAKE_BINARY_DIR}/bin/Debug>$<$<NOT:$<CONFIG:DEBUG>>:${CMAKE_BINARY_DIR}/bin/Release>")
    
    set_target_properties(${TargetName} 
        PROPERTIES VS_DEBUGGER_ENVIRONMENT 
        "PATH=${PXR_ENV_PATHS};${PXR_DEBUGER_ENV};
        ${PXR_DEBUGER_PLUGINPATH}${CONFIG_PLUGIN_PATH}/plugins;")
endif()

#include(GoogleTest)
#gtest_discover_tests(${TargetName})