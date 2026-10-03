add_subdirectory(3rdparty_libs)

if(NAU_ENGINE_KERNEL)
  message(NOTICE "Building engine kernel")
  add_subdirectory(core/kernel)
  set_target_properties (NauKernel PROPERTIES FOLDER "${NauEngineFolder}/core")
endif()

if(NAU_CORE_TESTS)
  add_subdirectory(core/app_framework/tests)
endif()
