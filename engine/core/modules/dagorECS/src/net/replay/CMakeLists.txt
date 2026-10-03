
# CMakeList.txt : CMake project for NAU::gameLibs_daECS_net_replay 
# git: -

set(SOURCE 
    replay.cpp
    stubs.cpp
)



source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${SOURCE}


) 

source_group("PublicHeaders" FILES )

add_library(replay STATIC)
add_library(NAU::gameLibs_daECS_net_replay ALIAS replay)

target_sources(replay PRIVATE 
    ${SOURCE}



)


target_include_directories(replay PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    

)

target_link_libraries(replay 
    NAU::engine2_ioSys
    NAU::dagorInclude
)  
target_link_libraries(replay )

target_include_directories(replay PUBLIC 
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/engine/dagorInclude>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/gameLibs/publicInclude>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/3rdPartyLibs/rapidjson/include>

) 







add_nau_compile_options(replay)
#add_nau_folder_property(replay LIB gameLibs)

set_target_properties (replay PROPERTIES
    FOLDER "NAU/gameLibs/daECS/net/replay/"
)

install(TARGETS replay
    EXPORT replay    
    RUNTIME DESTINATION "bin"  
    ARCHIVE DESTINATION "lib/gameLibs/replay"
    LIBRARY DESTINATION "lib/gameLibs/replay"
)

install(EXPORT replay
    FILE replay-config.cmake
    NAMESPACE NAUE::
    DESTINATION "lib/cmake/gameLibs/replay"
)



