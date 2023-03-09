# ------------------------------------------------------------------------------
# Required EbsdLib and H5Support
# ------------------------------------------------------------------------------
find_package(H5Support REQUIRED)

find_package(EbsdLib REQUIRED)

# ------------------------------------------------------------------------------
# EbsdLib needs install rules for creating packages
# ------------------------------------------------------------------------------
get_property(GENERATOR_IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

#if(NOT GENERATOR_IS_MULTI_CONFIG)
  get_property(COMPLEX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY COMPLEX_EXTRA_LIBRARY_DIRS ${COMPLEX_EXTRA_LIBRARY_DIRS} ${EbsdLib_LIB_DIRS})
#endif()

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
  AlignSectionsMutualInformationFilter
  BadDataNeighborOrientationCheckFilter
  ConvertOrientations
  ConvertQuaternionFilter
  CreateEnsembleInfoFilter
  EBSDSegmentFeaturesFilter
  EbsdToH5EbsdFilter
  EnsembleInfoReaderFilter
  ExportGBCDGMTFileFilter
  ExportGBCDTriangleDataFilter
  FindAvgOrientationsFilter
  FindFeatureReferenceMisorientationsFilter
  FindGBCDFilter
  FindKernelAvgMisorientationsFilter
  FindMisorientationsFilter
  FindSchmidsFilter
  FindShapesFilter
  FindTriangleGeomShapesFilter
  GenerateFaceIPFColoringFilter
  GenerateFaceMisorientationColoringFilter
  GenerateFZQuaternions
  GenerateGBCDPoleFigureFilter
  GenerateIPFColorsFilter
  GenerateQuaternionConjugateFilter
  MergeTwinsFilter
  NeighborOrientationCorrelationFilter
  ReadAngDataFilter
  ReadCtfDataFilter
  ReadH5EbsdFilter
  RodriguesConvertorFilter
  RotateEulerRefFrameFilter
)
  
set(STUB_FILTERS

  # ConvertHexGridToSquareGrid # MISSING 1 or more Parameter Implementations
  # CreateEnsembleInfo # MISSING 1 or more Parameter Implementations
  CreateLambertSphere
  EMsoftSO3Sampler

  FindAvgCAxes
  FindBoundaryStrengths
  FindCAxisLocations
  FindDistsToCharactGBs
  FindFeatureNeighborCAxisMisalignments
  FindFeatureReferenceCAxisMisorientations

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

  # ImportEbsdMontage # MISSING 1 or more Parameter Implementations
  # ImportH5EspritData # MISSING 1 or more Parameter Implementations
  # ImportH5OimData # MISSING 1 or more Parameter Implementations
  # OrientationUtility # MISSING 1 or more Parameter Implementations

  #ImportEbsdMontage # MISSING 1 or more Parameter Implementations
  #ImportH5EspritData # MISSING 1 or more Parameter Implementations
  #ImportH5OimData # MISSING 1 or more Parameter Implementations
  
  #OrientationUtility # MISSING 1 or more Parameter Implementations

  WritePoleFigure
  WriteStatsGenOdfAngleFile
)

create_complex_plugin(NAME ${PLUGIN_NAME}
  FILTER_LIST ${FilterList}
  DESCRIPTION "${PLUGIN_NAME} Plugin"
  VERSION "0.2.0"
  DOC_CHECK
  ADD_TO_GLOBAL_LIST
)

# ------------------------------------------------------------------------------
# Add any filter parameters to the `complex` target
# This should be integrated with the `create_complex_plugin` function call
#
# The `complex` target will need to know about any dependent libraries and any
# include directories that are needed for the parameter.
# ------------------------------------------------------------------------------
set(complex_injected_parameters
  H5EbsdReaderParameter
  #EbsdToH5EbsdParameter
)

foreach(source ${complex_injected_parameters})
  target_sources(complex PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.h
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.cpp)

  source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME}
    FILES ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.h
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.cpp)
endforeach()

target_include_directories(complex PUBLIC )

# ------------------------------------------------------------------------------
# Add a link library to complex because it now depends on EbsdLib. Complex will
# be able to find the include dirs from the CMake EbsdLib target
# ------------------------------------------------------------------------------
target_link_libraries(complex PUBLIC EbsdLib::EbsdLib)

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
  AlignSectionsMutualInformation
  BadDataNeighborOrientationCheck
  ConvertQuaternion
  CreateEnsembleInfo
  EBSDSegmentFeatures
  EbsdToH5Ebsd
  EnsembleInfoReader
  ExportGBCDGMTFile
  ExportGBCDTriangleData
  FindAvgOrientations
  FindFeatureReferenceMisorientations
  FindGBCD
  FindKernelAvgMisorientations
  FindMisorientations
  FindSchmids
  FindShapes
  FindTriangleGeomShapes
  GenerateFaceIPFColoring
  GenerateFaceMisorientationColoring
  GenerateGBCDPoleFigure
  GenerateIPFColors
  GenerateQuaternionConjugate
  GroupFeatures
  MergeTwins
  NeighborOrientationCorrelation
  ReadAngData
  ReadCtfData
  ReadH5Ebsd
  RodriguesConvertor
  RotateEulerRefFrame
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
# Extra Sources
# ------------------------------------------------------------------------------
target_sources(${PLUGIN_NAME}
               PRIVATE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Math/Matrix3X3.hpp"
               "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Math/Matrix3X1.hpp")
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Math" PREFIX ${PLUGIN_NAME}
             FILES "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Math/Matrix3X3.hpp"
                    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Math/Matrix3X1.hpp"
             )


set(PLUGIN_EXTRA_SOURCES
"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/inicpp.h"
"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/PhaseType.hpp"
"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/PhaseType.cpp"
)
target_sources(${PLUGIN_NAME} PRIVATE ${PLUGIN_EXTRA_SOURCES})
source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities" PREFIX ${PLUGIN_NAME} FILES ${PLUGIN_EXTRA_SOURCES})


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

# -----------------------------------------------------------------------
# Install example pipelines
# -----------------------------------------------------------------------
if(EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines")
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pipelines/${PLUGIN_NAME}")
  add_custom_target(${PLUGIN_NAME}_Pipeline_Folder_Copy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines"
    ${PIPELINE_DEST_DIR}
    COMMENT "Copying Pipeline Folder into Binary Directory")
  set_target_properties(${PLUGIN_NAME}_Pipeline_Folder_Copy PROPERTIES FOLDER ZZ_COPY_FILES)

  set(INSTALL_DESTINATION "pipelines/${PLUGIN_NAME}")
  install(DIRECTORY
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/"
    DESTINATION "${INSTALL_DESTINATION}"
    COMPONENT Applications
  )
endif()

file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)

# -----------------------------------------------------------------------
# Download Example Data Files
# -----------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Utility.cmake)
set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/Data")

# -----------------------------------------------------------------------
# Copy the data files from their downloaded location into the `Data`
# directory of the CMAKE_LIBRARY_OUTPUT_DIRECTORY
# -----------------------------------------------------------------------
download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME T12-MAI-2010.tar.gz SHA512 e33f224d19ad774604aa28a3263a00221a3a5909040685a3d14b6cba78e36d174b045223c28b462ab3eaea0fbc1c9f0657b1bd791a947799b9f088b13d777568)
download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Textured_Copper.tar.gz SHA512 cb231a13abbc21fe2046fc0fc7be9753ea51440bce1827bd20ee225499cd38be07d67955ff8c09d15ad91c4c1934e2a0e34406a703e7e49fbe53068b29ef440e)
download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100.tar.gz SHA512 79e9f6948d4e8e06187e11216a67596fa786ffd2700e51f594ad014090383eb8bcc003e14de2e88082aa9ae512cc4fc9cee22c80066fc54f38c3ebc75267eb5b)

set(DREAM3D_DATA_ARCHIVES "Small_IN100;T12-MAI-2010;Textured_Copper")
foreach(archive ${DREAM3D_DATA_ARCHIVES})
  add_custom_target(Copy_${archive} ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DREAM3D_DATA_DIR}/TestFiles/${archive} ${DATA_DEST_DIR}/${archive}
    COMMENT "Copying ${archive} into Binary Directory"
    DEPENDS Fetch_${archive})
  set_target_properties(Copy_${archive} PROPERTIES FOLDER ZZ_COPY_FILES)

  install(DIRECTORY 
    ${DREAM3D_DATA_DIR_NORM}/TestFiles/${archive}
    DESTINATION Data/
    COMPONENT Applications
  )
endforeach()
