# Target-scoped options shared by browser runtime targets and bundled sources.
set(Platform_Emscripten ON)
set(Compiler_Clang ON)
set(Target_Arch wasm32)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "Nau browser build options currently support wasm32 only.")
endif()

if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR NOT EMSCRIPTEN)
    message(FATAL_ERROR "Nau browser targets require the Emscripten Clang toolchain.")
endif()

function(nau_add_emscripten_options target)
    cmake_parse_arguments(OPTIONS "STRICT;ENABLE_RTTI" "" "" ${ARGN})
    get_target_property(targetType ${target} TYPE)
    get_target_property(imported ${target} IMPORTED)
    if(imported)
        message(FATAL_ERROR "Browser build options require a source target, not imported target ${target}.")
    endif()
    if(targetType STREQUAL "SHARED_LIBRARY" OR targetType STREQUAL "MODULE_LIBRARY")
        message(FATAL_ERROR "Browser target ${target} must use static linking.")
    endif()

    if(targetType STREQUAL "INTERFACE_LIBRARY")
        set(visibility INTERFACE)
    else()
        set(visibility PUBLIC)
    endif()
    # PUBLIC also carries the threaded ABI to consumers. Each compiled dependency
    # must receive this helper; options on an executable do not flow backwards.
    target_compile_options(${target} ${visibility} -pthread)
    target_link_options(${target} ${visibility} -pthread)
    target_compile_features(${target} ${visibility} cxx_std_20)

    if(targetType STREQUAL "INTERFACE_LIBRARY")
        return()
    endif()
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_options(${target} PRIVATE
        "$<$<CONFIG:Debug>:-O0;-g3>"
        "$<$<CONFIG:Release>:-O3>"
    )
    if(NOT NAU_EXCEPTIONS)
        target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>")
    endif()
    if(NOT OPTIONS_ENABLE_RTTI)
        target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>")
    endif()
    if(OPTIONS_STRICT)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    endif()
    target_compile_definitions(${target} PRIVATE NAU_TARGET_NAME="${target}")

    if(targetType STREQUAL "EXECUTABLE")
        set_target_properties(${target} PROPERTIES SUFFIX ".js")
        target_link_options(${target} PRIVATE
            -sPROXY_TO_PTHREAD=1
            # Retained global-property helpers use a 64 KiB local allocator.
            # Leave room for its callers and coroutine/serialization frames.
            -sSTACK_SIZE=1048576
            "$<$<CONFIG:Debug>:-O0;-g3;-sASSERTIONS=2>"
            "$<$<CONFIG:Release>:-O3>"
        )
    endif()
endfunction()
