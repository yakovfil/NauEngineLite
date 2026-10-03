cmake_minimum_required(VERSION 4.2)
project(NauStandardLibraryProbes LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
get_filename_component(NAU_ENGINE_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
include(CTest)

add_executable(test_resource_allocator test_resource_allocator.cpp)
target_include_directories(test_resource_allocator PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME resource_allocator COMMAND test_resource_allocator)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME resource_allocator_overflow COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_allocation_failure.py" $<TARGET_FILE:test_resource_allocator>)
set_tests_properties(resource_allocator resource_allocator_overflow PROPERTIES TIMEOUT 30)

add_executable(test_lru_cache test_lru_cache.cpp)
target_include_directories(test_lru_cache PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME lru_cache COMMAND test_lru_cache)
add_test(NAME lru_zero_capacity COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_lru_failure.py" $<TARGET_FILE:test_lru_cache>)
set_tests_properties(lru_cache lru_zero_capacity PROPERTIES TIMEOUT 45)

add_executable(test_intrusive_ptr test_intrusive_ptr.cpp)
target_include_directories(test_intrusive_ptr PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME intrusive_ptr COMMAND test_intrusive_ptr)
set_tests_properties(intrusive_ptr PROPERTIES TIMEOUT 30)

add_executable(test_intrusive_list test_intrusive_list.cpp)
target_include_directories(test_intrusive_list PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME intrusive_list COMMAND test_intrusive_list)
set_tests_properties(intrusive_list PROPERTIES TIMEOUT 30)

add_executable(test_fixed_move_only_function test_fixed_move_only_function.cpp)
target_include_directories(test_fixed_move_only_function PRIVATE "${NAU_ENGINE_ROOT}/engine/core/modules/render/include")
add_test(NAME fixed_move_only_function COMMAND test_fixed_move_only_function)
add_test(NAME fixed_move_only_empty COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_empty_function.py" $<TARGET_FILE:test_fixed_move_only_function>)
set_tests_properties(fixed_move_only_function fixed_move_only_empty PROPERTIES TIMEOUT 45)

add_executable(test_fixed_function test_fixed_function.cpp)
target_include_directories(test_fixed_function PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME fixed_function COMMAND test_fixed_function)
set_tests_properties(fixed_function PROPERTIES TIMEOUT 30)

add_executable(test_dynamic_bit_vector test_dynamic_bit_vector.cpp)
target_include_directories(test_dynamic_bit_vector PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME dynamic_bit_vector COMMAND test_dynamic_bit_vector)
set_tests_properties(dynamic_bit_vector PROPERTIES TIMEOUT 30)

add_executable(test_inline_vector test_inline_vector.cpp)
target_include_directories(test_inline_vector PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME inline_vector COMMAND test_inline_vector)
add_test(NAME inline_capacity COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_inline_capacity.py" $<TARGET_FILE:test_inline_vector>)
set_tests_properties(inline_vector inline_capacity PROPERTIES TIMEOUT 45)

add_executable(test_inline_string test_inline_string.cpp)
target_include_directories(test_inline_string PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME inline_string COMMAND test_inline_string)
add_test(NAME inline_string_capacity COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/verify_inline_capacity.py" $<TARGET_FILE:test_inline_string>)
set_tests_properties(inline_string inline_string_capacity PROPERTIES TIMEOUT 30)

if(WIN32)
    foreach(origin allocation_origin_a allocation_origin_b)
        add_library(${origin} SHARED allocation_origin.cpp)
        target_include_directories(${origin} PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
    endforeach()
    add_executable(test_allocation_modules test_allocation_modules.cpp)
    target_include_directories(test_allocation_modules PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
    add_dependencies(test_allocation_modules allocation_origin_a allocation_origin_b)
    add_test(NAME allocation_modules COMMAND test_allocation_modules
        $<TARGET_FILE:allocation_origin_a> $<TARGET_FILE:allocation_origin_b>)
    set_tests_properties(allocation_modules PROPERTIES TIMEOUT 30)
endif()

add_executable(test_sorted_vector test_sorted_vector.cpp)
target_include_directories(test_sorted_vector PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME sorted_vector COMMAND test_sorted_vector)
set_tests_properties(sorted_vector PROPERTIES TIMEOUT 30)

add_executable(test_dag_vector test_dag_vector.cpp)
target_include_directories(test_dag_vector PRIVATE
    "${NAU_ENGINE_ROOT}/engine/core/kernel/include"
    "${NAU_ENGINE_ROOT}/engine/core/modules/dagorECS/include"
    "${NAU_ENGINE_ROOT}/engine/3rdparty_libs/fmt/include")
target_compile_definitions(test_dag_vector PRIVATE NAU_STATIC_RUNTIME NAU_KERNEL_EXPORT= FMT_HEADER_ONLY)
add_test(NAME dag_vector COMMAND test_dag_vector)
set_tests_properties(dag_vector PROPERTIES TIMEOUT 30)

add_executable(test_fixed_map test_fixed_map.cpp)
target_include_directories(test_fixed_map PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME fixed_map COMMAND test_fixed_map)
add_test(NAME fixed_map_capacity COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/verify_fixed_map_capacity.py" $<TARGET_FILE:test_fixed_map>)
set_tests_properties(fixed_map fixed_map_capacity PROPERTIES TIMEOUT 30)

add_executable(test_tuple_vector test_tuple_vector.cpp)
target_include_directories(test_tuple_vector PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
add_test(NAME tuple_vector COMMAND test_tuple_vector)
set_tests_properties(tuple_vector PROPERTIES TIMEOUT 30)

add_executable(test_kernel_utilities test_kernel_utilities.cpp)
target_include_directories(test_kernel_utilities PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include")
target_compile_definitions(test_kernel_utilities PRIVATE NAU_STATIC_RUNTIME NAU_64BIT=1)
add_test(NAME kernel_utilities COMMAND test_kernel_utilities)
set_tests_properties(kernel_utilities PROPERTIES TIMEOUT 30)

# Compile public utility headers separately, with no EASTL include directories.
foreach(header IN ITEMS nau/kernel/kernel_config.h nau/utils/enum_utils.h nau/utils/dag_stlqsort.h nau/generic/dag_patchTab.h nau/utils/span.h nau/utils/cancellation.h)
    string(MAKE_C_IDENTIFIER "${header}" unit)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${unit}.cpp" "#include <${header}>\n")
    target_sources(test_kernel_utilities PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/${unit}.cpp")
endforeach()

add_executable(test_hashed_map test_hashed_map.cpp)
target_include_directories(test_hashed_map PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include" "${NAU_ENGINE_ROOT}/engine/3rdparty_libs/wyhash")
add_test(NAME hashed_map COMMAND test_hashed_map)
foreach(kind IN ITEMS 1 2)
    add_library(hash_reject_${kind} OBJECT EXCLUDE_FROM_ALL test_hashed_map.cpp)
    target_compile_definitions(hash_reject_${kind} PRIVATE NAU_HASH_REJECT=${kind})
    target_include_directories(hash_reject_${kind} PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include" "${NAU_ENGINE_ROOT}/engine/3rdparty_libs/wyhash")
endforeach()

add_executable(test_name_map test_name_map.cpp "${NAU_ENGINE_ROOT}/engine/core/kernel/src/osApiWrappers/localCmp.cpp")
target_compile_definitions(test_name_map PRIVATE NAU_STATIC_RUNTIME)
target_include_directories(test_name_map PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include" "${NAU_ENGINE_ROOT}/engine/3rdparty_libs/wyhash")
add_test(NAME name_map COMMAND test_name_map)
set_tests_properties(name_map PROPERTIES TIMEOUT 30)

# These production string and filesystem helpers require only the standard library and fmt.
add_executable(test_file_list test_file_list.cpp)
target_include_directories(test_file_list PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/src/osApiWrappers")
add_test(NAME file_list COMMAND test_file_list)
set_tests_properties(file_list PROPERTIES TIMEOUT 30)

add_executable(test_format_views test_format_views.cpp)
target_include_directories(test_format_views PRIVATE
    "${NAU_ENGINE_ROOT}/engine/core/kernel/include"
    "${NAU_ENGINE_ROOT}/engine/3rdparty_libs/fmt/include")
target_compile_definitions(test_format_views PRIVATE FMT_HEADER_ONLY)
add_test(NAME format_views COMMAND test_format_views)
set_tests_properties(format_views PROPERTIES TIMEOUT 30)

add_executable(test_ska_hash_map test_ska_hash_map.cpp)
target_include_directories(test_ska_hash_map PRIVATE "${NAU_ENGINE_ROOT}/engine/core/kernel/include" "${NAU_ENGINE_ROOT}/engine/3rdparty_libs/ska_hash_map")
add_test(NAME ska_hash_map COMMAND test_ska_hash_map)
set_tests_properties(ska_hash_map PROPERTIES TIMEOUT 30)
