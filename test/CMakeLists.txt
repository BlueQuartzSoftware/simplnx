file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}" COMPLEX_SOURCE_DIR_NORM)
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}" COMPLEX_BINARY_DIR_NORM)

set(COMPLEX_TEST_DIRS_HEADER ${COMPLEX_GENERATED_HEADER_DIR}/unit_test/complex_test_dirs.hpp)

configure_file(${complex_SOURCE_DIR}/test/complex_test_dirs.hpp.in ${COMPLEX_TEST_DIRS_HEADER} @ONLY)

find_package(Catch2 CONFIG REQUIRED)

include(Catch)

get_property(COMPLEX_PLUGIN_COUNT GLOBAL PROPERTY COMPLEX_PLUGIN_COUNT)
add_subdirectory(${complex_SOURCE_DIR}/src/Plugins/TestOne ${complex_BINARY_DIR}/Plugins/TestOne)
add_subdirectory(${complex_SOURCE_DIR}/src/Plugins/TestTwo ${complex_BINARY_DIR}/Plugins/TestTwo)
MATH(EXPR COMPLEX_PLUGIN_COUNT "${COMPLEX_PLUGIN_COUNT}+2")

add_subdirectory(UnitTestCommon)

add_executable(complex_test 
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
  DREAM3DFileTest.cpp
  GeometryTestUtilities.hpp
  ParametersTest.cpp
  PipelineSaveTest.cpp
)

target_link_libraries(complex_test
  PRIVATE
    complex
    ComplexCore
    Catch2::Catch2
    complex::UnitTestCommon
)

# Require that the test plugins are built before tests because some tests
# require loading from those plugins but don't want to link to them.
add_dependencies(complex_test TestOne TestTwo)

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

# -----------------------------------------------------------------------
# Download the Test Data
# -----------------------------------------------------------------------
include(FetchContent)

function(download_dream3d_test_files url hash)
  message(STATUS "Downloading DREAM3D_Data/TestFiles")
  FetchContent_Declare(download_DREAM3D_Data
    URL ${url}
    URL_HASH SHA512=${hash}
    SOURCE_DIR "${DREAM3D_DATA_DIR}/TestFiles"
    DOWNLOAD_DIR "${DREAM3D_DATA_DIR}"

    # BINARY_DIR "${DREAM3D_DATA_DIR}/Binary"
    # INSTALL_DIR "${DREAM3D_DATA_DIR}/Install"
    # TMP_DIR "${DREAM3D_DATA_DIR}/tmp"
    # STAMP_DIR "${DREAM3D_DATA_DIR}/stamp"
  )

  FetchContent_MakeAvailable(download_DREAM3D_Data)
endfunction(download_dream3d_test_files)

if(EXISTS "${DREAM3D_DATA_DIR}")
  message(STATUS "DREAM3D Data: Found at '${DREAM3DNXProj_PARENT}/DREAM3D_Data/'")
  set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data/")
  add_custom_target(DataFolderCopy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DREAM3D_DATA_DIR}/Data
    ${DATA_DEST_DIR}
    COMMENT "Copying Data Folder into Binary Directory")
  set_target_properties(DataFolderCopy PROPERTIES FOLDER ZZ_COPY_FILES)

  add_custom_target(H5EbsdDataCopy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DREAM3D_DATA_DIR}/TestFiles/SmallIN100.h5ebsd
    ${DATA_DEST_DIR}/H5Ebsd/Small_IN100.h5ebsd
    COMMENT "Copying Data Folder into Binary Directory")
  set_target_properties(H5EbsdDataCopy PROPERTIES FOLDER ZZ_COPY_FILES)  

  set(DREAM3D_DATA_DIRECTORIES
    ${DREAM3D_DATA_DIR}/Data/Image
    ${DREAM3D_DATA_DIR}/Data/Models
    ${DREAM3D_DATA_DIR}/Data/H5Ebsd
  )

  set(INSTALL_DESTINATION "Data")

  # NOTE: If we are creating an Anaconda install the install directory WILL be different
  foreach(data_dir ${DREAM3D_DATA_DIRECTORIES})
    if(EXISTS ${data_dir})
      install(DIRECTORY
        ${data_dir}
        DESTINATION ${INSTALL_DESTINATION}
        COMPONENT Applications
      )
    endif()
  endforeach()

  download_dream3d_test_files(
    https://github.com/dream3d/DREAM3D_Data/releases/download/v6_8/TestFiles_6_8.tar.gz
    17af96674010f07e77758ee2335199e866a3c036b8df9755f203b85a8ad1112c19a5a110539c634b0b2cefce73c76c999becc1a4661205925852e07f278b287e
  )

else()
  message(STATUS "DREAM3D Data: NOT found at '${DREAM3DNXProj_PARENT}/DREAM3D_Data/'. Data will not be available for testing")
endif()

