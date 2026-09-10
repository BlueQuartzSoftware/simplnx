file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" SIMPLNX_SOURCE_DIR_NORM)
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}" SIMPLNX_BINARY_DIR_NORM)

#------------------------------------------------------------------------------
# Create the targets to download the test files
#------------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/DownloadTestData.cmake")

set(SIMPLNX_TEST_DIRS_HEADER ${SIMPLNX_GENERATED_HEADER_DIR}/unit_test/simplnx_test_dirs.hpp)

configure_file(${simplnx_SOURCE_DIR}/test/simplnx_test_dirs.hpp.in ${SIMPLNX_TEST_DIRS_HEADER} @ONLY)
file(MAKE_DIRECTORY "${simplnx_BINARY_DIR}/simplnx/test_output")

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

add_subdirectory(UnitTestCommon)
add_subdirectory(ConversionTest)

add_executable(simplnx_test
  ${SIMPLNX_TEST_DIRS_HEADER}
  simplnx_test_main.cpp
  AppTest.cpp
  ArgumentsTest.cpp
  BitTest.cpp
  DataArrayTest.cpp
  DataPathTest.cpp
  DataStructObserver.hpp
  DataStructObserver.cpp
  DataStructTest.cpp
  DelayVfd.hpp
  DelayVfd.cpp
  Dream3dPreflightCacheTest.cpp
  DynamicFilterInstantiationTest.cpp
  FilePathGeneratorTest.cpp
  GeometryTest.cpp
  GeometryTestUtilities.hpp
  H5Test.cpp
  IntersectionUtilitiesTest.cpp
  IOFormat.cpp
  MessageHandlerTest.cpp
  MontageTest.cpp
  NeighborUtilitiesTest.cpp
  PluginTest.cpp
  ParametersTest.cpp
  PipelineFilterMessagingTest.cpp
  PipelineSaveTest.cpp
  UuidTest.cpp
  Vec3Test.cpp
  StringUtilitiesTest.cpp
  ThrottledMessageHandlerTest.cpp
  ScaleBarRendererTest.cpp
  FilterValidationTest.cpp
  SimplJsonConversionTest.cpp
)

target_link_libraries(simplnx_test
  PRIVATE
    simplnx
    Catch2::Catch2
    simplnx::UnitTestCommon
)

simplnx_enable_warnings(TARGET simplnx_test)

#------------------------------------------------------------------------------
# Require that the test plugins are built before tests because some tests
# require loading from those plugins but don't want to link to them.
get_property(SIMPLNX_PLUGIN_COUNT GLOBAL PROPERTY SIMPLNX_PLUGIN_COUNT)

#------------------------------------------------------------------------------
# Create dependencies with simplnx_test on each of the test plugins
#------------------------------------------------------------------------------
if(SIMPLNX_PLUGIN_ENABLE_TestOne)
  add_dependencies(simplnx_test TestOne)
endif()

if(SIMPLNX_PLUGIN_ENABLE_TestTwo)
  add_dependencies(simplnx_test TestTwo)
endif()

set_target_properties(simplnx_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:simplnx>
)

target_compile_definitions(simplnx_test
  PRIVATE
    SIMPLNX_BUILD_DIR="$<TARGET_FILE_DIR:simplnx_test>"
    SIMPLNX_PLUGIN_COUNT=${SIMPLNX_PLUGIN_COUNT}
)

target_compile_options(simplnx_test
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

target_include_directories(simplnx_test PRIVATE ${SIMPLNX_GENERATED_DIR})

catch_discover_tests(simplnx_test)



if(EXISTS "${DREAM3D_DATA_DIR}" AND SIMPLNX_DOWNLOAD_TEST_FILES) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME simpl_json_exemplars.tar.gz SHA512 550a1ad3c9c31b1093d961fdda2057d63a48f08c04d2831c70c1ec65d499f39b354a5a569903ff68e9b703ef45c23faba340aa0c61f9a927fbc845c15b87d05e )
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME itk_image_reader_test_v3.tar.gz SHA512 e583911e69958f5a270f72f22a07527af313c1e3870923f1c95f1f55bb7c242d9e99daeed7100bfa1f5b4af37c952cf98abe0eb2486c1ec5248470b0ca2dcb85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME import_image_stack_test_v3.tar.gz SHA512 e9305ac5c3592129cba937bd426c3d987a020a05229cf4519d826bcc538d054b4a173d613ed3e72b7646a523f2f3874c7dab39404cc0ddabffa653cba845abd5)
endif()

add_custom_target(Copy_simplnx_Test_Data ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${simplnx_SOURCE_DIR}/test/Data" "${DATA_DEST_DIR}/Test_Data"
    WORKING_DIRECTORY "${simplnx_SOURCE_DIR}/test"
    COMMENT "Copying ${simplnx_SOURCE_DIR}/test/Data data into Binary Directory"
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
set_target_properties(Copy_simplnx_Test_Data PROPERTIES FOLDER ZZ_COPY_FILES)
