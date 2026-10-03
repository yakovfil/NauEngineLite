
# CMakeList.txt : CMake project for NAU::gameLibs_daECS_net 
# git: -

set(PUBLIC_HEADERS
    ../../publicInclude/daECS/net/compBlacklist.h
    ../../publicInclude/daECS/net/component_replication_filter.h
    ../../publicInclude/daECS/net/compver.h
    ../../publicInclude/daECS/net/connection.h
    ../../publicInclude/daECS/net/connid.h
    ../../publicInclude/daECS/net/dasEvents.h
    ../../publicInclude/daECS/net/message.h
    ../../publicInclude/daECS/net/msgDecl.h
    ../../publicInclude/daECS/net/msgSink.h
    ../../publicInclude/daECS/net/netbase.h
    ../../publicInclude/daECS/net/netEvent.h
    ../../publicInclude/daECS/net/netEvents.h
    ../../publicInclude/daECS/net/network.h
    ../../publicInclude/daECS/net/object.h
    ../../publicInclude/daECS/net/recipientFilters.h
    ../../publicInclude/daECS/net/reliabilitySys.h
    ../../publicInclude/daECS/net/replay.h
    ../../publicInclude/daECS/net/replayEvents.h
    ../../publicInclude/daECS/net/schemelessEventSerialize.h
    ../../publicInclude/daECS/net/scope_query_cb.h
    ../../publicInclude/daECS/net/sequence.h
    ../../publicInclude/daECS/net/serialize.h
    ../../publicInclude/daECS/net/time.h
)
set(SOURCE
    compBlacklist.cpp
    component_replication_filter.cpp
    connection.cpp
    daNetDriver.cpp
    message.cpp
    network.cpp
    object.cpp
    recipientFilters.cpp
    reliabilitySys.cpp
    schemelessEventSerialize.cpp
    serialize.cpp
    utils.cpp
)

set(inl
    replicationES.cpp.inl
    msgSinkES.cpp.inl
    netEventES.cpp.inl
    netEventsES.cpp.inl
    componentFiltersES.cpp.inl
)

set(HEADERS
    component_replication_filters.h
    compression.h
    encryption.h
    objectReplica.h
    utils.h
)



source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${SOURCE}
    ${HEADERS}

    #<user-code:extra_source_group

    #>
)

source_group("PublicHeaders" FILES ${PUBLIC_HEADERS}
)

add_library(gameLibs_daECS_net STATIC)
add_library(NAU::gameLibs_daECS_net ALIAS gameLibs_daECS_net)

target_sources(gameLibs_daECS_net PRIVATE
    ${SOURCE}
    ${HEADERS}

    ${PUBLIC_HEADERS}

    #<user-code:extra_source

    #>
)


target_include_directories(gameLibs_daECS_net PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    
    #<user-code:extra_includes

    #>
)

target_link_libraries(gameLibs_daECS_net
    NAU::dagorInclude
    NAU::gameLibs_daNet
    NAU::gameLibs_crypto
    #NAU::gameLibs_daECS_gameres
    NAU::gameLibs_daECS_io_datablock
    NAU::gameLibs_daECS_core

    lz4
)
target_link_libraries(gameLibs_daECS_net )

target_include_directories(gameLibs_daECS_net PUBLIC)



nau_gen_es(gameLibs_daECS_net esFiles esPull ${inl})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES
    ${esFiles}
    ${esPull}
)
target_sources(gameLibs_daECS_net PRIVATE
    ${esFiles}
    ${esPull}
)

add_nau_compile_options(gameLibs_daECS_net)
#add_nau_folder_property(gameLibs_daECS_net LIB gameLibs)

set_target_properties(gameLibs_daECS_net PROPERTIES
    FOLDER "${NauEngineFolder}/NAU/gameLibs/daECS/net/"
)

install(TARGETS gameLibs_daECS_net
    EXPORT gameLibs_daECS_net
    RUNTIME DESTINATION "bin"
    ARCHIVE DESTINATION "lib/gameLibs/net"
    LIBRARY DESTINATION "lib/gameLibs/net"
)

if(DEFINED HEADERS)
    install(FILES ${HEADERS} ${PUBLIC_HEADERS}
        DESTINATION "include/gameLibs/net")
endif()

install(EXPORT gameLibs_daECS_net
    FILE gameLibs_daECS_net-config.cmake
    NAMESPACE NAUE::
    DESTINATION "lib/cmake/gameLibs/net"
)

add_subdirectory(dasevents)
add_subdirectory(replay)
add_subdirectory(timesync)


