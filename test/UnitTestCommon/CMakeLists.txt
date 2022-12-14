find_package(Catch2 CONFIG REQUIRED)

add_library(ComplexUnitTestCommon INTERFACE)
add_library(complex::UnitTestCommon ALIAS ComplexUnitTestCommon)

target_include_directories(ComplexUnitTestCommon
  INTERFACE
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include
)

target_link_libraries(ComplexUnitTestCommon
  INTERFACE
    complex
    Catch2::Catch2
)

target_sources(ComplexUnitTestCommon
  INTERFACE
    ${PROJECT_SOURCE_DIR}/test/UnitTestCommon/include/complex/UnitTest/UnitTestCommon.hpp
)
