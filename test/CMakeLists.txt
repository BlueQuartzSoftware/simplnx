file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" COMPLEX_SOURCE_DIR_NORM)
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}" COMPLEX_BINARY_DIR_NORM)

#------------------------------------------------------------------------------
# Create the targets to download the test files
#------------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/DownloadTestData.cmake")

set(COMPLEX_TEST_DIRS_HEADER
  ${COMPLEX_GENERATED_HEADER_DIR}/unit_test/complex_test_dirs.hpp
  ChunkDataStore.hpp
)

configure_file(${complex_SOURCE_DIR}/test/complex_test_dirs.hpp.in ${COMPLEX_TEST_DIRS_HEADER} @ONLY)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_subdirectory(UnitTestCommon)

add_executable(complex_test
  ${COMPLEX_TEST_DIRS_HEADER}
  complex_test_main.cpp
  ArgumentsTest.cpp
  BitTest.cpp
  DataArrayTest.cpp
  DataPathTest.cpp
  DataStructObserver.hpp
  DataStructObserver.cpp
  DataStructTest.cpp
  DynamicFilterInstantiationTest.cpp
  FilePathGeneratorTest.cpp
  GeometryTest.cpp
  GeometryTestUtilities.hpp
  H5Test.cpp
  IOFormat.cpp
  MontageTest.cpp
  PluginTest.cpp
  ParametersTest.cpp
  PipelineSaveTest.cpp
  UuidTest.cpp
)

target_link_libraries(complex_test
  PRIVATE
    complex
    Catch2::Catch2
    complex::UnitTestCommon
)

#------------------------------------------------------------------------------
# Require that the test plugins are built before tests because some tests
# require loading from those plugins but don't want to link to them.
get_property(COMPLEX_PLUGIN_COUNT GLOBAL PROPERTY COMPLEX_PLUGIN_COUNT)

#------------------------------------------------------------------------------
# Create dependencies with complex_test on each of the test plugins
#------------------------------------------------------------------------------
if(COMPLEX_PLUGIN_ENABLE_TestOne)
  add_dependencies(complex_test TestOne)
endif()

if(COMPLEX_PLUGIN_ENABLE_TestTwo)
  add_dependencies(complex_test TestTwo)
endif()

set_target_properties(complex_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)

target_compile_definitions(complex_test
  PRIVATE
    COMPLEX_BUILD_DIR="$<TARGET_FILE_DIR:complex_test>"
    COMPLEX_PLUGIN_COUNT=${COMPLEX_PLUGIN_COUNT}
)

target_compile_options(complex_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_include_directories(complex_test PRIVATE ${COMPLEX_GENERATED_DIR})

catch_discover_tests(complex_test)


