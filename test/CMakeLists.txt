
file(TO_CMAKE_PATH "${complex_SOURCE_DIR}" complex_SOURCE_DIR_NORM)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/complex_test_dirs.h.in
                ${COMPLEX_GENERATED_HEADER_DIR}/unit_test/complex_test_dirs.h @ONLY )


find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_subdirectory(Plugin)

add_executable(complex_test
  MACOSX_BUNDLE
  ${complex_BINARY_DIR}/complex_test_dirs.h
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

set_target_properties(complex_test
  PROPERTIES
    BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER com.BlueQuartzSoftware.style.complex.Tests
    MACOSX_BUNDLE_BUNDLE_NAME "Complex Tests"
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in
)

target_compile_options(complex_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_include_directories(complex_test PRIVATE ${COMPLEX_GENERATED_DIR})

# add_custom_command(
#   TARGET complex_test POST_BUILD
#   COMMAND ${CMAKE_COMMAND} -E copy
#     ${CMAKE_SOURCE_DIR}/test/data/SmallN100.dream3d
#     $<TARGET_FILE_DIR:complex_test>/test/data/SmallN100.dream3d
# )

catch_discover_tests(complex_test)
