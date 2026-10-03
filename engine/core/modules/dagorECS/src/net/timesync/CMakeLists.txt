
# CMakeList.txt : CMake project for NAU::gameLibs_daECS_net_timesync 
# git: -

set(SOURCE 
    accumTime.cpp
    clientTime.cpp
    serverTime.cpp
)



source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES 
    ${SOURCE}


) 

source_group("PublicHeaders" FILES )

add_library(timesync STATIC)
add_library(NAU::gameLibs_daECS_net_timesync ALIAS timesync)

target_sources(timesync PRIVATE 
    ${SOURCE}



)


target_include_directories(timesync PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    

)

target_link_libraries(timesync  NAU::dagorInclude)

target_include_directories(timesync PUBLIC 
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/engine/dagorInclude>
	$<BUILD_INTERFACE:${NAU_ENGINE_ROOT}/prog/gameLibs/publicInclude>

) 







add_nau_compile_options(timesync)
#add_nau_folder_property(timesync LIB gameLibs)

set_target_properties (timesync PROPERTIES
    FOLDER "${NauEngineFolder}/NAU/gameLibs/daECS/net/timesync/"
)

install(TARGETS timesync
    EXPORT timesync    
    RUNTIME DESTINATION "bin"  
    ARCHIVE DESTINATION "lib/gameLibs/timesync"
    LIBRARY DESTINATION "lib/gameLibs/timesync"
)

install(EXPORT timesync
    FILE timesync-config.cmake
    NAMESPACE NAUE::
    DESTINATION "lib/cmake/gameLibs/timesync"
)



