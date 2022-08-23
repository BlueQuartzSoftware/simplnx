# ------------------------------------------------------------------------------
# Required EbsdLib and H5Support
# ------------------------------------------------------------------------------
find_package(H5Support REQUIRED)

find_package(EbsdLib REQUIRED)

# ------------------------------------------------------------------------------
# EbsdLib needs install rules for creating packages
# ------------------------------------------------------------------------------
get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(NOT GENERATOR_IS_MULTI_CONFIG)
  get_property(COMPLEX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS ${COMPLEX_EXTRA_LIBRARY_DIRS} ${EbsdLib_LIB_DIRS})
endif()

# ------------------------------------------------------------------------------
# Include the standard complex plugin cmake support codes
# ------------------------------------------------------------------------------
include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "OrientationAnalysis")
set(${PLUGIN_NAME}_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# OrientationAnalysis/src/OrientationAnalysis/Filters/ directory.
set(FilterList
  AlignSectionsMisorientationFilter
  GenerateFZQuaternions
  ConvertOrientations
  ReadH5EbsdFilter
  RotateEulerRefFrameFilter
  EBSDSegmentFeaturesFilter
  FindAvgOrientationsFilter
  GenerateIPFColorsFilter
  BadDataNeighborOrientationCheckFilter
  NeighborOrientationCorrelationFilter
  MergeTwinsFilter

  FindFeatureReferenceMisorientationsFilter
  FindKernelAvgMisorientationsFilter
  FindMisorientationsFilter
  FindSchmidsFilter
)

set(STUB_FILTERS
  ConvertQuaternion

  # ConvertHexGridToSquareGrid # MISSING 1 or more Parameter Implementations
  # CreateEnsembleInfo # MISSING 1 or more Parameter Implementations
  CreateLambertSphere
  EMsoftSO3Sampler

  # EbsdToH5Ebsd # MISSING 1 or more Parameter Implementations
  EnsembleInfoReader
  FindAvgCAxes
  FindBoundaryStrengths
  FindCAxisLocations
  FindDistsToCharactGBs
  FindFeatureNeighborCAxisMisalignments
  FindFeatureReferenceCAxisMisorientations

  FindGBCD
  FindGBCDMetricBased
  FindGBPDMetricBased
  FindKernelAvgMisorientations
  FindMisorientations
  FindSchmids
  FindSlipTransmissionMetrics
  FindTwinBoundaries
  FindTwinBoundarySchmidFactors
  GenerateFaceIPFColoring
  GenerateFaceMisorientationColoring
  GenerateOrientationMatrixTranspose
  GenerateQuaternionConjugate
  INLWriter

  # ImportEbsdMontage # MISSING 1 or more Parameter Implementations
  # ImportH5EspritData # MISSING 1 or more Parameter Implementations
  # ImportH5OimData # MISSING 1 or more Parameter Implementations
  # OrientationUtility # MISSING 1 or more Parameter Implementations
  ReadCtfData
  ReplaceElementAttributesWithNeighborValues
  RodriguesConvertor
  Stereographic3D
  WritePoleFigure
  WriteStatsGenOdfAngleFile
)

# ------------------------------------------------------------------------------
foreach(file_name ${FilterList})
  string(REPLACE "Filter" "" file_name ${file_name})
  if(NOT EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/docs/${PLUGIN_NAME}/${file_name}.md")
    message(STATUS "!!!> DOCUMENTATION WARNING:${${PLUGIN_NAME}_SOURCE_DIR}/docs/${PLUGIN_NAME}/${file_name}.md Does Not Exist")
  endif()
endforeach()


create_complex_plugin(NAME ${PLUGIN_NAME}
  FILTER_LIST ${FilterList}
  DESCRIPTION "Plugin for ${PLUGIN_NAME}"
  VERSION "0.2.0"
)

# ------------------------------------------------------------------------------
# Add in all the parameters.
# This should be integrated with the `create_complex_plugin` function call
# ------------------------------------------------------------------------------
set(Parameter_List
  H5EbsdReaderParameter
)

foreach(source ${Parameter_List})
  target_sources(${PLUGIN_NAME} PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.h
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.cpp)

  source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME}
    FILES ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.h
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.cpp)
endforeach()

# ------------------------------------------------------------------------------
# Include filters that need to be compiled but not available to the general user
# These are typically auto-generated sources that need to be compiled to ensure
# the stubs are following the latest API changes
# ------------------------------------------------------------------------------
foreach(file_name ${STUB_FILTERS})
  list(APPEND hidden_filter_HDRS
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters_Private/${file_name}.hpp"
  )
  list(APPEND hidden_filter_SRCS
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters_Private/${file_name}.cpp"
  )
endforeach()

option(${PLUGIN_NAME}_COMPILE_PRIVATE_FILTERS "Compile the Stubbed out filters" OFF)
if(${${PLUGIN_NAME}_COMPILE_PRIVATE_FILTERS})
  target_sources(${PLUGIN_NAME}
    PRIVATE ${hidden_filter_HDRS} ${hidden_filter_SRCS}
  )
  source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${hidden_filter_HDRS} ${hidden_filter_SRCS})
endif()

# ------------------------------------------------------------------------------
# Algorithms
# This should be integrated with the `create_complex_plugin` function call
# ------------------------------------------------------------------------------
set(filter_algorithms
  AlignSectionsMisorientation
  ReadH5Ebsd
  RotateEulerRefFrame
  EBSDSegmentFeatures
  FindAvgOrientations
  GenerateIPFColors
  BadDataNeighborOrientationCheck
  NeighborOrientationCorrelation
  MergeTwins
  GroupFeatures

  FindFeatureReferenceMisorientations
  FindKernelAvgMisorientations
  FindMisorientations
  FindSchmids
)


foreach(algorithm ${filter_algorithms})
  target_sources(${PLUGIN_NAME}
    PRIVATE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/Algorithms/${algorithm}.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/Algorithms/${algorithm}.hpp"
  )
  source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME}
    FILES "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/Algorithms/${algorithm}.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/Algorithms/${algorithm}.hpp"
  )
endforeach()

# ------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# ------------------------------------------------------------------------------
target_link_libraries(${PLUGIN_NAME} PUBLIC EbsdLib::EbsdLib)

# ------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
target_include_directories(${PLUGIN_NAME}
  PUBLIC
    "${EbsdLib_INCLUDE_DIRS}"
    "${${PLUGIN_NAME}_BINARY_DIR}"
)

# ------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
