if(SIMPLNX_ENABLE_OrientationAnalysis)
  file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" SIMPLNX_SOURCE_DIR_NORM)
  file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}" SIMPLNX_BINARY_DIR_NORM)

  set(SIMPLNX_TEST_DIRS_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/unit_test/simplnx_test_dirs.hpp)

  configure_file(${simplnx_SOURCE_DIR}/test/simplnx_test_dirs.hpp.in ${SIMPLNX_TEST_DIRS_HEADER} @ONLY)
  file(MAKE_DIRECTORY "${simplnx_BINARY_DIR}/simplnx/conversion_test_output")

  find_package(Catch2 CONFIG REQUIRED)

  include(Catch)

  add_executable(conversion_test
    ${SIMPLNX_TEST_DIRS_HEADER}
    conversion_test_main.cpp
    BackwardsCompatibilityTest.cpp
  )

  target_link_libraries(conversion_test
    PRIVATE
      simplnx::simplnx
      Catch2::Catch2
      simplnx::UnitTestCommon
      simplnx::OrientationAnalysis
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

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME backwards_compatibility_test.tar.gz SHA512 5684c4fc1833366e380b1a54da90a6edec0097f7b39a1dde0f2ce3cf7613ab9b64a94508745eb0345b0fde9dc8c5464f0785b5e38117fb9020b72ad9409b0249 )
endif()