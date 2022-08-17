# ------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  AlignSectionsFeatureCentroidTest.cpp 
  ConvertColorToGrayScaleTest.cpp
  FillBadDataTest.cpp # NEEDS TEST IMPLEMENTED
  FindFeatureCentroidsFilterTest.cpp
  FindShapesFilterTest.cpp
  ResampleImageGeomFilterTest.cpp # NEEDS TEST IMPLEMENTED
  RotateSampleRefFrameTest.cpp
  ErodeDilateBadDataTest.cpp
  WriteASCIIDataTest.cpp
  WriteBinaryDataTest.cpp
)

create_complex_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
  FILTER_LIST ${${PLUGIN_NAME}UnitTest_SRCS})

# ------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC [name of library])

# ------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}UnitTest
#  PRIVATE ${${PLUGIN_NAME}_SOURCE_DIR}/test/${PLUGIN_NAME}TestUtilities.hpp
# )

# ------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
target_include_directories(${PLUGIN_NAME}UnitTest
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/../../src
)
