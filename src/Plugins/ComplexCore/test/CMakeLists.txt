
file(TO_CMAKE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}" PLUGIN_SOURCE_DIR_NORM)

set(${PLUGIN_NAME}_GENERATED_DIR ${complex_BINARY_DIR}/Plugins/${PLUGIN_NAME}/generated)
set(COMPLEX_TEST_DIRS_HEADER ${${PLUGIN_NAME}_GENERATED_DIR}/${PLUGIN_NAME}/${PLUGIN_NAME}_test_dirs.hpp)

configure_file(${${PLUGIN_NAME}_SOURCE_DIR}/test/test_dirs.hpp.in ${COMPLEX_TEST_DIRS_HEADER} @ONLY)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)


add_executable(${PLUGIN_NAME}UnitTest
  ${COMPLEX_TEST_DIRS_HEADER}
  ${PLUGIN_NAME}_test_main.cpp
  CreateDataArrayTest.cpp
  )

target_link_libraries(${PLUGIN_NAME}UnitTest
  PRIVATE
  complex
  ComplexCore
  Catch2::Catch2
  )

# Require that the test plugins are built before tests because some tests
# require loading from those plugins but don't want to link to them.
add_dependencies(${PLUGIN_NAME}UnitTest ${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME}UnitTest
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
  )

target_compile_definitions(${PLUGIN_NAME}UnitTest
  PRIVATE
  COMPLEX_BUILD_DIR="$<TARGET_FILE_DIR:complex_test>"
  )

target_compile_options(${PLUGIN_NAME}UnitTest
  PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/MP>
  )

target_include_directories(${PLUGIN_NAME}UnitTest PRIVATE ${${PLUGIN_NAME}_GENERATED_DIR})

catch_discover_tests(${PLUGIN_NAME}UnitTest)

