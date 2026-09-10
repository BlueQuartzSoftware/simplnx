project(apps
  VERSION 0.1.0
  DESCRIPTION "simplnx command line applications"
  LANGUAGES CXX
)

# -----------------------------------------------------------------------
# Define install destination directories
# -----------------------------------------------------------------------
set(DEST_DIR ".") # Windows we dump everything into the top level dir

if((UNIX AND NOT APPLE) OR SIMPLNX_CONDA_BUILD) # Linux we obey standard unix layout
  set(DEST_DIR "bin") # Apple we have a custom script somewhere
endif()

# -----------------------------------------------------------------------
# Applies the properties, link libraries, install rules and Linux RPATH
# settings that every simplnx CLI application shares.
# -----------------------------------------------------------------------
function(simplnx_configure_cli_app TARGET_NAME)
  set_target_properties(${TARGET_NAME}
    PROPERTIES
      DEBUG_POSTFIX "${SIMPLNX_DEBUG_POSTFIX}"
  )

  target_link_libraries(${TARGET_NAME} PRIVATE simplnx::simplnx)

  install(TARGETS ${TARGET_NAME}
    COMPONENT Applications
    RUNTIME DESTINATION ${DEST_DIR}
    LIBRARY DESTINATION ${DEST_DIR}
    ARCHIVE DESTINATION ${DEST_DIR}
    BUNDLE DESTINATION ${DEST_DIR}
  )

  if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set_target_properties(${TARGET_NAME}
      PROPERTIES
        INSTALL_RPATH \$ORIGIN/../lib
    )
    target_link_options(${TARGET_NAME} PUBLIC "-Wl,--disable-new-dtags")
    target_compile_options(${TARGET_NAME} PUBLIC "-no-pie")
  endif()
endfunction()

# -----------------------------------------------------------------------
# nxrunner: preflight / execute / convert pipelines
# -----------------------------------------------------------------------
set(NXRUNNER_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.hpp
)

set(NXRUNNER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nxrunner.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/CliObserver.cpp
)

add_executable(nxrunner)

target_sources(nxrunner
  PRIVATE
    ${NXRUNNER_HDRS}
    ${NXRUNNER_SRCS}
)

source_group("nxrunner" FILES ${NXRUNNER_HDRS} ${NXRUNNER_SRCS})

simplnx_configure_cli_app(nxrunner)

# Applies the shared warning-to-error set, including -Werror=unused-result, so a discarded
# [[nodiscard]] Result in the command line runner fails the build.
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
simplnx_enable_warnings(TARGET nxrunner)

if(BUILD_TESTING)
  add_test(
    NAME "nxrunner cache memory budget CLI"
    COMMAND ${CMAKE_COMMAND}
      "-DNXRUNNER_EXECUTABLE=$<TARGET_FILE:nxrunner>"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/test/CacheMemoryBudgetCliTest.cmake"
  )
endif()

if(SIMPLNX_EMBED_PYTHON)
  find_package(pybind11 2.10.0 CONFIG REQUIRED)

  target_link_libraries(nxrunner
    PRIVATE
      NxPythonEmbed
  )
endif()

# -----------------------------------------------------------------------
# nxinfo: report information about simplnx related files
# -----------------------------------------------------------------------
set(NXINFO_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nxinfo.cpp
)

add_executable(nxinfo)

target_sources(nxinfo
  PRIVATE
    ${NXINFO_SRCS}
)

source_group("nxinfo" FILES ${NXINFO_SRCS})

simplnx_configure_cli_app(nxinfo)

# -----------------------------------------------------------------------
# nxinfo ctest entries. The input file is produced by the simplnx_test case
# "DataStructure::exportHierarchyAsJson::PreflightRoundTrip", so these tests
# depend on it through a ctest fixture.
# -----------------------------------------------------------------------
if(SIMPLNX_BUILD_TESTS)
  # This directory is configured before the top-level include(CTest) call.
  enable_testing()

  set(NXINFO_TEST_INPUT "${simplnx_BINARY_DIR}/simplnx/test_output/exportHierarchyAsJson_roundtrip.dream3d")
  set(NXINFO_TEST_OUTPUT "${simplnx_BINARY_DIR}/simplnx/test_output/nxinfo_dump.json")
  set(NXINFO_EXE "$<TARGET_FILE:nxinfo>")
  set(NXINFO_WORKING_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

  add_test(NAME "NXINFO::GenerateRoundTripFile"
    COMMAND $<TARGET_FILE:simplnx_test> "DataStructure::exportHierarchyAsJson::PreflightRoundTrip"
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  set_tests_properties("NXINFO::GenerateRoundTripFile" PROPERTIES FIXTURES_SETUP NxInfoRoundTripFile)

  add_test(NAME NXINFO::DumpJson COMMAND ${NXINFO_EXE} -d ${NXINFO_TEST_INPUT} WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::DumpJson PROPERTIES PASS_REGULAR_EXPRESSION "\"schema_version\": 1")

  add_test(NAME NXINFO::DumpText COMMAND ${NXINFO_EXE} -d ${NXINFO_TEST_INPUT} -f TEXT WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::DumpText PROPERTIES PASS_REGULAR_EXPRESSION "\\|--")

  add_test(NAME NXINFO::DumpDot COMMAND ${NXINFO_EXE} -d ${NXINFO_TEST_INPUT} -f dot WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::DumpDot PROPERTIES PASS_REGULAR_EXPRESSION "digraph DataGraph")

  add_test(NAME NXINFO::DumpToFile COMMAND ${NXINFO_EXE} -d ${NXINFO_TEST_INPUT} -o ${NXINFO_TEST_OUTPUT} WORKING_DIRECTORY ${NXINFO_WORKING_DIR})

  add_test(NAME NXINFO::MissingFile COMMAND ${NXINFO_EXE} -d ${simplnx_BINARY_DIR}/does_not_exist.dream3d WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::MissingFile PROPERTIES WILL_FAIL TRUE)

  add_test(NAME NXINFO::BadFormat COMMAND ${NXINFO_EXE} -d ${NXINFO_TEST_INPUT} -f XML WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::BadFormat PROPERTIES WILL_FAIL TRUE)

  add_test(NAME NXINFO::NoArguments COMMAND ${NXINFO_EXE} WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::NoArguments PROPERTIES WILL_FAIL TRUE)

  add_test(NAME NXINFO::Version COMMAND ${NXINFO_EXE} --version WORKING_DIRECTORY ${NXINFO_WORKING_DIR})
  set_tests_properties(NXINFO::Version PROPERTIES PASS_REGULAR_EXPRESSION "nxinfo: Version")

  # Order after the simplnx_test case that writes the input file.
  set_tests_properties(NXINFO::DumpJson NXINFO::DumpText NXINFO::DumpDot NXINFO::DumpToFile NXINFO::BadFormat
    PROPERTIES FIXTURES_REQUIRED NxInfoRoundTripFile)
endif()
