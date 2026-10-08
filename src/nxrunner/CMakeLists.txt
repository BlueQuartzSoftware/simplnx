project(nxrunner
  VERSION 0.1.0
  DESCRIPTION "nxrunner"
  LANGUAGES CXX
)

set(CLI_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.hpp
)

set(CLI_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nxrunner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.cpp
)

add_executable(nxrunner)

set_target_properties(nxrunner
  PROPERTIES
    DEBUG_POSTFIX "${SIMPLNX_DEBUG_POSTFIX}"
)

target_sources(nxrunner
  PRIVATE
    ${CLI_HDRS}
    ${CLI_SRCS}
)

target_link_libraries(nxrunner PRIVATE simplnx::simplnx)

# Applies the shared warning-to-error set, including -Werror=unused-result, so a discarded
# [[nodiscard]] Result in the command line runner fails the build.
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
simplnx_enable_warnings(TARGET nxrunner)

if(BUILD_TESTING)
  add_executable(P1PreferencesDefaultsProbe test/PreferencesDefaultsProbe.cpp)
  target_link_libraries(P1PreferencesDefaultsProbe PRIVATE simplnx::simplnx)
  simplnx_enable_warnings(TARGET P1PreferencesDefaultsProbe)
  add_test(NAME "nxrunner OOC threshold preferences"
    COMMAND ${CMAKE_COMMAND}
      "-DNXRUNNER_EXECUTABLE=$<TARGET_FILE:nxrunner>"
      "-DDEFAULTS_PROBE=$<TARGET_FILE:P1PreferencesDefaultsProbe>"
      "-DPIPELINE_FILE=${CMAKE_CURRENT_SOURCE_DIR}/test/ooc_failure_pipeline.d3dpipeline"
      "-DP1_ROOT_BASE=${CMAKE_CURRENT_BINARY_DIR}/P1ThresholdProfiles"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/test/OocThresholdPreferencesTest.cmake")
  set_tests_properties("nxrunner OOC threshold preferences" PROPERTIES RUN_SERIAL TRUE TIMEOUT 750)

  add_test(
    NAME "nxrunner cache memory budget CLI"
    COMMAND ${CMAKE_COMMAND}
      "-DNXRUNNER_EXECUTABLE=$<TARGET_FILE:nxrunner>"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/test/CacheMemoryBudgetCliTest.cmake"
  )

endif()

source_group("nxrunner" FILES ${CLI_HDRS} ${CLI_SRCS})

if(SIMPLNX_EMBED_PYTHON)
  find_package(pybind11 2.10.0 CONFIG REQUIRED)

  target_link_libraries(nxrunner
    PRIVATE
      NxPythonEmbed
  )
endif()

# -----------------------------------------------------------------------
# Define install destination directories
# -----------------------------------------------------------------------
set(DEST_DIR ".") # Windows we dump everything into the top level dir

if((UNIX AND NOT APPLE) OR SIMPLNX_CONDA_BUILD) # Linux we obey standard unix layout
  set(DEST_DIR "bin") # Apple we have a custom script somewhere
endif()

# -- Create an Install Rule for the main app bundle target
install(TARGETS nxrunner
  COMPONENT Applications
  RUNTIME DESTINATION ${DEST_DIR}
  LIBRARY DESTINATION ${DEST_DIR}
  ARCHIVE DESTINATION ${DEST_DIR}
  BUNDLE DESTINATION ${DEST_DIR}
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  set(CMAKE_INSTALL_RPATH "\$ORIGIN/../lib")
  set_target_properties(nxrunner
    PROPERTIES
    INSTALL_RPATH \$ORIGIN/../lib
  )
  target_link_options(nxrunner PUBLIC "-Wl,--disable-new-dtags")
  target_compile_options(nxrunner PUBLIC "-no-pie")
endif()
