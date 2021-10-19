file(TO_CMAKE_PATH "${complex_SOURCE_DIR}" complex_SOURCE_DIR_NORM)

set(COMPLEX_TEST_DIRS_HEADER ${COMPLEX_GENERATED_HEADER_DIR}/unit_test/complex_test_dirs.h)

configure_file(${complex_SOURCE_DIR}/test/complex_test_dirs.h.in ${COMPLEX_TEST_DIRS_HEADER} @ONLY)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_subdirectory(${complex_SOURCE_DIR}/src/Plugins/TestOne ${complex_BINARY_DIR}/Plugins/TestOne)
add_subdirectory(${complex_SOURCE_DIR}/src/Plugins/TestTwo ${complex_BINARY_DIR}/Plugins/TestTwo)

add_executable(complex_test 
  # MACOSX_BUNDLE
  ${COMPLEX_TEST_DIRS_HEADER}
  complex_test_main.cpp
  ArgumentsTest.cpp
  DataStructTest.cpp
  GeometryTest.cpp
  H5Test.cpp
  DataStructObserver.hpp
  DataStructObserver.cpp
  MontageTest.cpp
  BitTest.cpp
  UuidTest.cpp
  CoreFilterTest.cpp
  PipelineTest.cpp
  PluginTest.cpp
  FilePathGeneratorTest.cpp
  DataArrayTest.cpp
  GeometryTestUtilities.hpp
  ParametersTest.cpp
)

target_link_libraries(complex_test
  PRIVATE
    complex
    Catch2::Catch2
)

set_target_properties(complex_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)

# set_target_properties(complex_test
#   PROPERTIES
#     BUNDLE True
#     MACOSX_BUNDLE_GUI_IDENTIFIER com.BlueQuartzSoftware.style.complex.Tests
#     MACOSX_BUNDLE_BUNDLE_NAME "Complex Tests"
#     MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
#     MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
#     MACOSX_BUNDLE_INFO_PLIST ${complex_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in
# )

target_compile_options(complex_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_include_directories(complex_test PRIVATE ${COMPLEX_GENERATED_DIR})



catch_discover_tests(complex_test)
