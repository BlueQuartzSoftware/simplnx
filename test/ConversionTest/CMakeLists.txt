
file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" SIMPLNX_SOURCE_DIR_NORM)
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}" SIMPLNX_BINARY_DIR_NORM)



set(SIMPLNX_TEST_DIRS_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/unit_test/simplnx_test_dirs.hpp)
message(STATUS "SIMPLNX_BUILD_DIR: ${SIMPLNX_BUILD_DIR}")

#------------------------------------------------------------------------------
# Populate with all plugin source dirs
set (SIMPLNX_PLUGIN_SOURCE_DIRS "const std::vector<fs::path> k_PluginSourceDirs = {")
get_property(simplnxPluginTargets GLOBAL PROPERTY simplnxPluginTargets)
foreach(d3dPlugin ${SIMPLNX_PLUGIN_LIST})
    set (SIMPLNX_PLUGIN_SOURCE_DIRS "${SIMPLNX_PLUGIN_SOURCE_DIRS}\n  fs::path(\"${${d3dPlugin}_SOURCE_DIR}\"),")
endforeach()
set (SIMPLNX_PLUGIN_SOURCE_DIRS "${SIMPLNX_PLUGIN_SOURCE_DIRS}\n};\n")

set(SIMPLNX_BUILD_DIR "${SIMPLNX_BINARY_DIR_NORM}")

configure_file(${simplnx_SOURCE_DIR}/test/simplnx_test_dirs.hpp.in ${SIMPLNX_TEST_DIRS_HEADER} @ONLY)
file(MAKE_DIRECTORY "${SIMPLNX_BINARY_DIR_NORM}/simplnx/conversion_test_output")

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_executable(conversion_test
  ${SIMPLNX_TEST_DIRS_HEADER}
  conversion_test_main.cpp
  SIMPLMapInventoryTest.cpp
)

target_link_libraries(conversion_test
  PRIVATE
    simplnx::simplnx
    Catch2::Catch2
    simplnx::UnitTestCommon
)

simplnx_enable_warnings(TARGET conversion_test)



#------------------------------------------------------------------------------
# Require that the test plugins are built before tests because some tests
# require loading from those plugins but don't want to link to them.
get_property(SIMPLNX_PLUGIN_COUNT GLOBAL PROPERTY SIMPLNX_PLUGIN_COUNT)

set_target_properties(conversion_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:simplnx>
)

target_compile_definitions(conversion_test
  PRIVATE
    SIMPLNX_BUILD_DIR="$<TARGET_FILE_DIR:conversion_test>"
    SIMPLNX_PLUGIN_COUNT=${SIMPLNX_PLUGIN_COUNT}
)

target_compile_options(conversion_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_include_directories(conversion_test PRIVATE ${SIMPLNX_GENERATED_DIR})

catch_discover_tests(conversion_test)

