find_package(Catch2 CONFIG REQUIRED)
find_package(reproc++ CONFIG REQUIRED)

add_library(ComplexUnitTestCommon INTERFACE)
add_library(simplnx::UnitTestCommon ALIAS ComplexUnitTestCommon)

target_include_directories(ComplexUnitTestCommon
  INTERFACE
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include
)

target_link_libraries(ComplexUnitTestCommon
  INTERFACE
    simplnx
    Catch2::Catch2
    reproc++
)

target_sources(ComplexUnitTestCommon
  INTERFACE
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/simplnx/UnitTest/UnitTestCommon.hpp
)
