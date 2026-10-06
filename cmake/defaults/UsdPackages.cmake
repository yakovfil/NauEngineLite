# file to find packages
set(NAU_USD_SDK_DIR "" CACHE PATH "Locally built OpenUSD SDK (upstream install layout)")
if(NAU_USD_SDK_DIR)
    set(pxr_DIR "${NAU_USD_SDK_DIR}")
    if(NOT EXISTS "${pxr_DIR}/pxrConfig.cmake")
        message(FATAL_ERROR "Missing local OpenUSD SDK: ${pxr_DIR}/pxrConfig.cmake")
    endif()
else()
    include(FetchContent)
    FetchContent_Declare(Usd
        URL https://cdn.nauengine.org/s3/naungn-portal-prod/documents/usd24.08_no_Hydra.zip
        SOURCE_DIR exdeps/usd)
    FetchContent_MakeAvailable(Usd)
    FetchContent_GetProperties(Usd SOURCE_DIR UsdDir)
    cmake_path(SET pxr_DIR NORMALIZE "${UsdDir}")
    set(Python3_EXECUTABLE "${pxr_DIR}/python/python.exe")
    set(Python3_LIBRARY "${pxr_DIR}/python/libs/python310.lib")
    set(Python3_INCLUDE_DIR "${pxr_DIR}/python/include")
    set(MaterialX_DIR "${pxr_DIR}/lib/cmake/MaterialX")
    set(Imath_DIR "${pxr_DIR}/lib/cmake/Imath")
endif()
find_package(pxr REQUIRED)
if(WIN32)
    set_property(TARGET tf APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS BOOST_ALL_NO_LIB)
endif()
set(PXR_LIB_DIR "${PXR_CMAKE_DIR}/lib")
set(PXR_NODEFAULTLIBS /NODEFAULTLIB:boost_python310-vc143-mt-gd-x64-1_78.lib /NODEFAULTLIB:boost_python310-vc143-mt-x64-1_78.lib)
set(PXR_ENV_PATHS "${PXR_CMAKE_DIR}/bin;${PXR_CMAKE_DIR}/lib;${PXR_CMAKE_DIR}/python;${PXR_CMAKE_DIR}/plugin/usd;$<$<CONFIG:DEBUG>:${_IMPORT_PREFIX}${PXR_CMAKE_DIR}/lib/debug>$<$<NOT:$<CONFIG:DEBUG>>:${_IMPORT_PREFIX}${PXR_CMAKE_DIR}/lib/release>;$<$<CONFIG:DEBUG>:${_IMPORT_PREFIX}${PXR_CMAKE_DIR}/plugin/debug/usd>$<$<NOT:$<CONFIG:DEBUG>>:${_IMPORT_PREFIX}${PXR_CMAKE_DIR}/plugin/release/usd>;")

set(PXR_DEBUGER_ENV "
TF_DEBUG=PLUG_REGISTRATION
PYTHONPATH=${PXR_CMAKE_DIR}/lib/python;
PXR_MTLX_STDLIB_SEARCH_PATHS=${PXR_CMAKE_DIR}/libraries;"
)

set(PXR_DEBUGER_PLUGINPATH "
PXR_PLUGINPATH_NAME=$<$<CONFIG:DEBUG>:${_IMPORT_PREFIX}${PXR_CMAKE_DIR}/plugin/debug/usd>$<$<NOT:$<CONFIG:DEBUG>>:${_IMPORT_PREFIX}${PXR_CMAKE_DIR}/plugin/release/usd>;${PXR_CMAKE_DIR}/plugin/usd;"
)

set(USD_GENSCHEMA "${PXR_CMAKE_DIR}/bin/usdGenSchema.cmd")

if(NAU_USD_SDK_DIR)
    get_filename_component(NAU_USD_PYTHON_DIR "${Python3_EXECUTABLE}" DIRECTORY)
    set(NAU_USD_PYTHON_DLL "${NAU_USD_PYTHON_DIR}/python310.dll")
    set(NAU_USD_PYTHONPATH "${PXR_CMAKE_DIR}/lib/python")
    set(NAU_USD_TOOL_PATH "${NAU_USD_PYTHON_DIR};${PXR_CMAKE_DIR}/bin;${PXR_LIB_DIR}")
    set(PXR_NODEFAULTLIBS "")
    set(PXR_ENV_PATHS "${NAU_USD_TOOL_PATH};")
    set(PXR_DEBUGER_PLUGINPATH "PXR_PLUGINPATH_NAME=${PXR_LIB_DIR}/usd;${PXR_CMAKE_DIR}/plugin/usd;")
else()
    set(NAU_USD_PYTHON_DLL "${PXR_CMAKE_DIR}/Python/python310.dll")
    set(NAU_USD_PYTHONPATH "${PXR_CMAKE_DIR}/lib/release/python")
    set(NAU_USD_TOOL_PATH "${PXR_CMAKE_DIR}/bin;${PXR_LIB_DIR};${PXR_LIB_DIR}/release;${PXR_CMAKE_DIR}/python;${PXR_CMAKE_DIR}/plugin/usd")
endif()

function(nau_process_usd_schema target shemaFile output_files)

    set (OUTPUT_BASE "${CMAKE_CURRENT_BINARY_DIR}/generated_src/schema_plugins")
    set (OUTPUT_DIR "${OUTPUT_BASE}/nau/${target}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/configure_plugInfo.cmake" "configure_file(\"${OUTPUT_DIR}/plugInfo.json.in\" \"${OUTPUT_DIR}/plugInfo.json\" @ONLY)")

    if(NOT EXISTS "${OUTPUT_DIR}/generatedSchema.usda")
        message("generate schema ${CMAKE_CURRENT_SOURCE_DIR}/${shemaFile} to ${OUTPUT_DIR}")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E env
                "PYTHONPATH=${NAU_USD_PYTHONPATH}"
                "PATH=${NAU_USD_TOOL_PATH}"
                ${USD_GENSCHEMA} "${CMAKE_CURRENT_SOURCE_DIR}/${shemaFile}" "${OUTPUT_DIR}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
        execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OUTPUT_DIR}/plugInfo.json ${OUTPUT_DIR}/plugInfo.json.in)
        execute_process(
            COMMAND ${CMAKE_COMMAND} -DPLUG_INFO_ROOT=${PLUG_INFO_ROOT} -DPLUG_INFO_RESOURCE_PATH=${PLUG_INFO_RESOURCE_PATH} -DPLUG_INFO_LIBRARY_PATH=${PLUG_INFO_LIBRARY_PATH}
                -P "${CMAKE_CURRENT_BINARY_DIR}/configure_plugInfo.cmake"
            WORKING_DIRECTORY "${OUTPUT_DIR}"
        )
    endif()
    target_include_directories(${target} PUBLIC ${OUTPUT_BASE})
    nau_collect_files(${output_files}
        DIRECTORIES ${OUTPUT_DIR}
        MASK "*.cpp" "*.h" "*.usda" "*.json"
    )

    add_custom_command(OUTPUT ${${output_files}}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${shemaFile}"
        COMMAND ${CMAKE_COMMAND} -E env
                "PYTHONPATH=${NAU_USD_PYTHONPATH}"
                "PATH=${NAU_USD_TOOL_PATH}"
                ${USD_GENSCHEMA} "${CMAKE_CURRENT_SOURCE_DIR}/${shemaFile}" "${OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OUTPUT_DIR}/plugInfo.json ${OUTPUT_DIR}/plugInfo.json.in
        COMMAND ${CMAKE_COMMAND} -DPLUG_INFO_ROOT=${PLUG_INFO_ROOT} -DPLUG_INFO_RESOURCE_PATH=${PLUG_INFO_RESOURCE_PATH} -DPLUG_INFO_LIBRARY_PATH=${PLUG_INFO_LIBRARY_PATH}
            -P "${CMAKE_CURRENT_BINARY_DIR}/configure_plugInfo.cmake"
        WORKING_DIRECTORY "${OUTPUT_DIR}"
    )

    set(${output_files} ${${output_files}} PARENT_SCOPE)
endfunction(nau_process_usd_schema)
