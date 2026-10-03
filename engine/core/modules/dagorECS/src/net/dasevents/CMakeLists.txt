
# CMakeList.txt : CMake project for NAU::gameLibs_daECS_net_dasevents 
# git: -

set(SOURCE
    dasEventsSQ.cpp
)
set(inl
	dasEventES.cpp.inl
)



source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${SOURCE}


) 

source_group("PublicHeaders" FILES )

add_library(gameLibs_daECS_net_dasevents STATIC)
add_library(NAU::gameLibs_daECS_net_dasevents ALIAS gameLibs_daECS_net_dasevents)

target_sources(gameLibs_daECS_net_dasevents PRIVATE
    ${SOURCE}



)


target_include_directories(gameLibs_daECS_net_dasevents PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    

)

target_link_libraries(gameLibs_daECS_net_dasevents

	NAU::dagorInclude
)

target_include_directories(gameLibs_daECS_net_dasevents PUBLIC
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/engine2/dagorInclude>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/gameLibs/publicInclude>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/1stPartyLibs/daScript/include>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/1stPartyLibs/quirrel/sqrat/include>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/1stPartyLibs/quirrel/quirrel/include>

) 

nau_gen_es(gameLibs_daECS_net_dasevents esFiles esPull ${inl})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
	${esFiles}
	${esPull}
)
target_sources(gameLibs_daECS_net_dasevents PRIVATE
	${esFiles}
	${esPull}
)

target_compile_definitions(gameLibs_daECS_net_dasevents PRIVATE
	-DUSE_SQRAT_CONFIG 

) 
target_compile_definitions(gameLibs_daECS_net_dasevents PRIVATE
    -DDAS_SKA_HASH=0
)



add_nau_compile_options(gameLibs_daECS_net_dasevents)
#add_nau_folder_property(gameLibs_daECS_net_dasevents LIB gameLibs)

set_target_properties (gameLibs_daECS_net_dasevents PROPERTIES
    FOLDER "${NauEngineFolder}/NAU/gameLibs/daECS/net/dasevents/"
)

install(TARGETS gameLibs_daECS_net_dasevents
    EXPORT gameLibs_daECS_net_dasevents
    RUNTIME DESTINATION "bin"  
    ARCHIVE DESTINATION "lib/gameLibs/dasevents"
    LIBRARY DESTINATION "lib/gameLibs/dasevents"
)

install(EXPORT gameLibs_daECS_net_dasevents
    FILE gameLibs_daECS_net_dasevents-config.cmake
    NAMESPACE NAUE::
    DESTINATION "lib/cmake/gameLibs/dasevents"
)



