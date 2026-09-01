find_package(Catch2 CONFIG REQUIRED)

add_library(SimplnxUnitTestCommon INTERFACE)
add_library(simplnx::UnitTestCommon ALIAS SimplnxUnitTestCommon)

target_include_directories(SimplnxUnitTestCommon
  INTERFACE
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include
)

target_link_libraries(SimplnxUnitTestCommon
  INTERFACE
    simplnx
    Catch2::Catch2
    ZLIB::ZLIB
)

target_sources(SimplnxUnitTestCommon
  PUBLIC
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/AlgorithmTestScope.cpp
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/UnitTestCommon.cpp
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/HDF5DatasetProbe.cpp
  INTERFACE
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/AlgorithmTestScope.hpp
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/UnitTestCommon.hpp
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/DataObjectComparison.hpp
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/HDF5DatasetProbe.hpp
)
