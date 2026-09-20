if(NAU_RUNTIME_PROFILE STREQUAL "demo")
  # Actual demo executable is integrated in task 4.1.
  return()
endif()

if (NAU_CORE_SAMPLES)

  if(NAU_RUNTIME_PROFILE STREQUAL "minimal")
    add_subdirectory(minimalApp)
    return()
  endif()

  add_subdirectory("sample_common")

  nau_collect_cmake_subdirectories(sampleDirs ${CMAKE_CURRENT_SOURCE_DIR})

  foreach(sample ${sampleDirs})
    if (NOT ${sample} STREQUAL "sample_common")
      message(STATUS "Found sample: (${sample})")
      add_subdirectory(${sample})
    endif()
  endforeach()

  # Copy samples to prebuild engine
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${CMAKE_INSTALL_PREFIX}
  )
endif()
