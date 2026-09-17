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
    target_link_options(${TARGET_NAME} PRIVATE "-Wl,--disable-new-dtags" "-no-pie")
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
set(NXINFO_CORE_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/NxInfo.hpp
)

set(NXINFO_CORE_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/NxInfoRunner.cpp
)

set(NXINFO_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/nxinfo.cpp
)

add_library(nxinfo_core STATIC)

target_sources(nxinfo_core
  PRIVATE
    ${NXINFO_CORE_HDRS}
    ${NXINFO_CORE_SRCS}
)

target_include_directories(nxinfo_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(nxinfo_core PRIVATE simplnx::simplnx)
simplnx_enable_warnings(TARGET nxinfo_core)

add_executable(nxinfo)

target_sources(nxinfo
  PRIVATE
    ${NXINFO_SRCS}
)

source_group("nxinfo" FILES ${NXINFO_CORE_HDRS} ${NXINFO_CORE_SRCS} ${NXINFO_SRCS})

simplnx_configure_cli_app(nxinfo)
target_link_libraries(nxinfo PRIVATE nxinfo_core)
simplnx_enable_warnings(TARGET nxinfo)

# -----------------------------------------------------------------------
# nxinfo ctest entries. A hidden Catch2 test produces the input file, so these
# tests depend on it through a ctest fixture.
# -----------------------------------------------------------------------
if(SIMPLNX_BUILD_TESTS)
  # This directory is configured before the top-level include(CTest) call.
  enable_testing()

  set(NXINFO_TEST_INPUT "${SIMPLNX_TEST_OUTPUT_DIR}/nxinfo_ctest_input.dream3d")
  set(NXINFO_TEST_INVALID_INPUT "${SIMPLNX_TEST_OUTPUT_DIR}/nxinfo_ctest_invalid.dream3d")
  set(NXINFO_TEST_OUTPUT "${SIMPLNX_TEST_OUTPUT_DIR}/nxinfo_dump.json")
  set(NXINFO_EXE "$<TARGET_FILE:nxinfo>")
  set(NXINFO_WORKING_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")

  add_test(NAME "NXINFO::GenerateTestFile"
    COMMAND $<TARGET_FILE:simplnx_test> "NXInfo::GenerateTestFile"
    WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
  set_tests_properties("NXINFO::GenerateTestFile" PROPERTIES FIXTURES_SETUP NxInfoTestFile)

  add_test(NAME NXINFO::DumpJson COMMAND "${NXINFO_EXE}" -d "${NXINFO_TEST_INPUT}" WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::DumpJson PROPERTIES PASS_REGULAR_EXPRESSION "\"schema_version\": 1")

  add_test(NAME NXINFO::DumpText COMMAND "${NXINFO_EXE}" -d "${NXINFO_TEST_INPUT}" -f TEXT WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::DumpText PROPERTIES PASS_REGULAR_EXPRESSION "\\|--")

  add_test(NAME NXINFO::DumpDot COMMAND "${NXINFO_EXE}" -d "${NXINFO_TEST_INPUT}" -f dot WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::DumpDot PROPERTIES PASS_REGULAR_EXPRESSION "digraph DataGraph")

  add_test(NAME NXINFO::InvalidFileDiagnostics
    COMMAND ${CMAKE_COMMAND}
      "-DNXINFO_EXE=${NXINFO_EXE}"
      "-DNXINFO_INVALID_INPUT=${NXINFO_TEST_INVALID_INPUT}"
      -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/VerifyNxInfoInvalidFile.cmake"
    WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::InvalidFileDiagnostics PROPERTIES FIXTURES_REQUIRED NxInfoTestFile)

  add_test(NAME NXINFO::RemoveDumpFile COMMAND "${CMAKE_COMMAND}" -E rm -f "${NXINFO_TEST_OUTPUT}" WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::RemoveDumpFile PROPERTIES FIXTURES_SETUP NxInfoOutputFileClean)

  add_test(NAME NXINFO::DumpToFile COMMAND "${NXINFO_EXE}" -d "${NXINFO_TEST_INPUT}" -o "${NXINFO_TEST_OUTPUT}" WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::DumpToFile PROPERTIES FIXTURES_REQUIRED "NxInfoTestFile;NxInfoOutputFileClean" FIXTURES_SETUP NxInfoDumpFile)

  add_test(NAME NXINFO::VerifyDumpToFile COMMAND "${CMAKE_COMMAND}" -E cat "${NXINFO_TEST_OUTPUT}" WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::VerifyDumpToFile PROPERTIES FIXTURES_REQUIRED NxInfoDumpFile PASS_REGULAR_EXPRESSION "\"schema_version\": 1")

  add_test(NAME NXINFO::CleanupDumpFile COMMAND "${CMAKE_COMMAND}" -E rm -f "${NXINFO_TEST_OUTPUT}" WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::CleanupDumpFile PROPERTIES FIXTURES_CLEANUP NxInfoDumpFile)

  add_test(NAME NXINFO::Help COMMAND "${NXINFO_EXE}" --help WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::Help PROPERTIES PASS_REGULAR_EXPRESSION "Usage:")

  add_test(NAME NXINFO::Version COMMAND "${NXINFO_EXE}" --version WORKING_DIRECTORY "${NXINFO_WORKING_DIR}")
  set_tests_properties(NXINFO::Version PROPERTIES PASS_REGULAR_EXPRESSION "nxinfo: Version")

  # Order after the simplnx_test case that writes the input file.
  set_tests_properties(NXINFO::DumpJson NXINFO::DumpText NXINFO::DumpDot PROPERTIES FIXTURES_REQUIRED NxInfoTestFile)
endif()
