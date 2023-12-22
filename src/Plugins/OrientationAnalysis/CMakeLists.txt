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
  get_property(SIMPLNX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS)
  set_property(GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS ${SIMPLNX_EXTRA_LIBRARY_DIRS} ${EbsdLib_LIB_DIRS})
#endif()

# ------------------------------------------------------------------------------
# Include the standard simplnx plugin cmake support codes
# ------------------------------------------------------------------------------
include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "OrientationAnalysis")
set(${PLUGIN_NAME}_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# OrientationAnalysis/src/OrientationAnalysis/Filters/ directory.
set(FilterList
  AlignSectionsMisorientationFilter
  AlignSectionsMutualInformationFilter
  BadDataNeighborOrientationCheckFilter
  CAxisSegmentFeaturesFilter
  ConvertHexGridToSquareGridFilter
  ConvertOrientations
  ConvertQuaternionFilter
  CreateEnsembleInfoFilter
  EBSDSegmentFeaturesFilter
  EbsdToH5EbsdFilter
  FindAvgCAxesFilter
  FindAvgOrientationsFilter
  FindBoundaryStrengthsFilter
  FindCAxisLocationsFilter
  FindFeatureNeighborCAxisMisalignmentsFilter
  FindFeatureReferenceCAxisMisorientationsFilter
  FindFeatureReferenceMisorientationsFilter
  FindGBCDFilter
  FindGBCDMetricBasedFilter
  FindGBPDMetricBasedFilter
  FindKernelAvgMisorientationsFilter
  FindMisorientationsFilter
  FindSchmidsFilter
  FindShapesFilter
  FindSlipTransmissionMetricsFilter
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
  ReadEnsembleInfoFilter
  ReadH5EbsdFilter
  ReadH5EspritDataFilter
  ReadH5OimDataFilter
  RodriguesConvertorFilter
  RotateEulerRefFrameFilter
  WriteGBCDGMTFileFilter
  WriteGBCDTriangleDataFilter
  WriteINLFileFilter
  WritePoleFigureFilter
  WriteStatsGenOdfAngleFileFilter
)
  
set(STUB_FILTERS
  CreateLambertSphere
  EMsoftSO3Sampler
  FindBoundaryStrengths
  FindDistsToCharactGBs
  FindKernelAvgMisorientations
  FindMisorientations
  FindSchmids
  FindTwinBoundaries
  FindTwinBoundarySchmidFactors
  GenerateFaceIPFColoring
  GenerateFaceMisorientationColoring
  GenerateOrientationMatrixTranspose
  # ImportEbsdMontage # MISSING 1 or more Parameter Implementations
)

create_simplnx_plugin(NAME ${PLUGIN_NAME}
  FILTER_LIST ${FilterList}
  DESCRIPTION "${PLUGIN_NAME} Plugin"
  VERSION "0.2.0"
  DOC_CHECK
  ADD_TO_GLOBAL_LIST
)

# ------------------------------------------------------------------------------
# Add any filter parameters to the `simplnx` target
# This should be integrated with the `create_simplnx_plugin` function call
#
# The `simplnx` target will need to know about any dependent libraries and any
# include directories that are needed for the parameter.
# ------------------------------------------------------------------------------
set(simplnx_injected_parameters
  ReadH5EbsdFileParameter
  OEMEbsdScanSelectionParameter
  #EbsdToH5EbsdParameter
)

foreach(source ${simplnx_injected_parameters})
  target_sources(simplnx PRIVATE
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.h
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.cpp)

  source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME}
    FILES ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.h
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Parameters/${source}.cpp)
endforeach()

target_include_directories(simplnx PUBLIC )

# ------------------------------------------------------------------------------
# Add a link library to simplnx because it now depends on EbsdLib. Complex will
# be able to find the include dirs from the CMake EbsdLib target
# ------------------------------------------------------------------------------
target_link_libraries(simplnx PUBLIC EbsdLib::EbsdLib)

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
# This should be integrated with the `create_simplnx_plugin` function call
# ------------------------------------------------------------------------------
set(filter_algorithms
  AlignSectionsMisorientation
  AlignSectionsMutualInformation
  BadDataNeighborOrientationCheck
  CAxisSegmentFeatures
  ConvertHexGridToSquareGrid
  ConvertQuaternion
  CreateEnsembleInfo
  EBSDSegmentFeatures
  EbsdToH5Ebsd
  FindAvgCAxes
  FindAvgOrientations
  FindBoundaryStrengths
  FindCAxisLocations
  FindFeatureNeighborCAxisMisalignments
  FindFeatureReferenceCAxisMisorientations
  FindFeatureReferenceMisorientations
  FindGBCD
  FindGBCDMetricBased
  FindGBPDMetricBased
  FindKernelAvgMisorientations
  FindMisorientations
  FindSchmids
  FindShapes
  FindSlipTransmissionMetrics
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
  ReadEnsembleInfo
  ReadH5Ebsd
  ReadH5EspritData
  ReadH5OimData
  RodriguesConvertor
  RotateEulerRefFrame
  WriteGBCDGMTFile
  WriteGBCDTriangleData
  WriteINLFile
  WritePoleFigure
  WriteStatsGenOdfAngleFile
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
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/ReadH5Data.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/OrientationUtilities.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/OrientationUtilities.cpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/Fonts.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/SIMPLConversion.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/TiffWriter.hpp"
  "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utilities/TiffWriter.cpp"
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
    "${${PLUGIN_NAME}_SOURCE_DIR}/3rdParty/canvas_ity/src"
)

# ------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
if(SIMPLNX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
endif()

# -----------------------------------------------------------------------
# Install example pipelines
# -----------------------------------------------------------------------
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
  set(CX_CONFIG_DIR "$<CONFIG>")
else()
  set(CX_CONFIG_DIR ".")
endif()

if(EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines")
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/pipelines/${PLUGIN_NAME}")
  add_custom_target(Copy_${PLUGIN_NAME}_Pipeline_Folder ALL
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines"
                    ${PIPELINE_DEST_DIR}
                    COMMENT "Copying Pipeline Folder into Binary Directory"
                    COMMAND_EXPAND_LISTS
                    VERBATIM
                  )
  set_target_properties(Copy_${PLUGIN_NAME}_Pipeline_Folder PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  option(${PLUGIN_NAME}_INSTALL_PIPELINES "Enables install of ${PLUGIN_NAME} pipelines" ON)

  if(${PLUGIN_NAME}_INSTALL_PIPELINES)
    set(INSTALL_DESTINATION "pipelines/${PLUGIN_NAME}")
    install(DIRECTORY
      "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/"
      DESTINATION "${INSTALL_DESTINATION}"
      COMPONENT Applications
    )
  endif()
endif()

# -----------------------------------------------------------------------
# Download Example Data Files
# -----------------------------------------------------------------------
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)
set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data")

file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)

# -----------------------------------------------------------------------
# These data files are pulled from the 'simplnx data archive' on github,
# decompressed and then copied to the build folder. Install rules are also
# generated for each of the data sets.
# -----------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}" AND SIMPLNX_DOWNLOAD_TEST_FILES) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
                   ARCHIVE_NAME T12-MAI-2010.tar.gz
                   SHA512 e33f224d19ad774604aa28a3263a00221a3a5909040685a3d14b6cba78e36d174b045223c28b462ab3eaea0fbc1c9f0657b1bd791a947799b9f088b13d777568
                   INSTALL
                   )
  add_custom_target(Copy_${PLUGIN_NAME}_T12-MAI-2010 ALL
                   COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/T12-MAI-2010.tar.gz" 
                   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/T12-MAI-2010" "${DATA_DEST_DIR}/T12-MAI-2010"
                   COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/T12-MAI-2010"
                   WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
                   COMMENT "Copying ${PLUGIN_NAME}/T12-MAI-2010 data into Binary Directory"
                   DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
                   COMMAND_EXPAND_LISTS
                   VERBATIM
                 )
  set_target_properties(Copy_${PLUGIN_NAME}_T12-MAI-2010 PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} 
                    ARCHIVE_NAME Textured_Copper.tar.gz 
                    SHA512 cb231a13abbc21fe2046fc0fc7be9753ea51440bce1827bd20ee225499cd38be07d67955ff8c09d15ad91c4c1934e2a0e34406a703e7e49fbe53068b29ef440e
                   INSTALL
                   )
  add_custom_target(Copy_${PLUGIN_NAME}_Textured_Copper ALL
                   COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/Textured_Copper.tar.gz" 
                   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/Textured_Copper" "${DATA_DEST_DIR}/Textured_Copper"
                   COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/Textured_Copper"
                   WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
                   COMMENT "Copying ${PLUGIN_NAME}/Textured_Copper data into Binary Directory"
                   DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
                   COMMAND_EXPAND_LISTS
                   VERBATIM
                 )
  set_target_properties(Copy_${PLUGIN_NAME}_Textured_Copper PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} 
                    ARCHIVE_NAME Small_IN100.tar.gz 
                    SHA512 79e9f6948d4e8e06187e11216a67596fa786ffd2700e51f594ad014090383eb8bcc003e14de2e88082aa9ae512cc4fc9cee22c80066fc54f38c3ebc75267eb5b
                    INSTALL)
  add_custom_target(Copy_${PLUGIN_NAME}_Small_IN100 ALL
                   COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/Small_IN100.tar.gz" 
                   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/Small_IN100" "${DATA_DEST_DIR}/Small_IN100"
                   COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/Small_IN100"
                   WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
                   COMMENT "Copying ${PLUGIN_NAME}/Small_IN100 data into Binary Directory"
                   DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
                   COMMAND_EXPAND_LISTS
                   VERBATIM
                 )

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME 6_6_ImportH5Data.tar.gz
    SHA512 68cf620d28a515d04040d66c4f8fa9d8d46707c65138b366b47f40d1d56a36c40e2e2cd0c9d35168b68c2cb5b2ce95650fa2a0d4e6ffce980d0dd0654908f40d
  )
  add_custom_target(Copy_${PLUGIN_NAME}_H5Data ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/6_6_ImportH5Data.tar.gz"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DREAM3D_DATA_DIR}/TestFiles/6_6_ImportH5Data/EdaxOIMData.h5" "${DREAM3D_DATA_DIR}/TestFiles/6_6_ImportH5Data/H5EspritReaderTest.h5" "${DATA_DEST_DIR}/${PLUGIN_NAME}/"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/6_6_ImportH5Data"
    WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
    COMMENT "Copying ${PLUGIN_NAME} data into Binary Directory"
    DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_H5Data PROPERTIES FOLDER ZZ_COPY_FILES)
endif()

# -----------------------------------------------------------------------
# Create build folder copy rules and install rules for the 'data' folder
# for this plugin
# -----------------------------------------------------------------------
add_custom_target(Copy_${PLUGIN_NAME}_Data ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${${PLUGIN_NAME}_SOURCE_DIR}/data ${DATA_DEST_DIR}/${PLUGIN_NAME}
  COMMENT "Copying ${PLUGIN_NAME} data into Binary Directory"
  COMMAND_EXPAND_LISTS
  VERBATIM
)
set_target_properties(Copy_${PLUGIN_NAME}_Data PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

option(${PLUGIN_NAME}_INSTALL_DATA_FILES "Enables install of ${PLUGIN_NAME} data files" ON)

set(Installed_Data_Files
  ${${PLUGIN_NAME}_SOURCE_DIR}/data/Ensemble.ini
  ${${PLUGIN_NAME}_SOURCE_DIR}/data/fz_quats.raw
  ${${PLUGIN_NAME}_SOURCE_DIR}/data/quats.raw
)

if(${PLUGIN_NAME}_INSTALL_DATA_FILES)
  install(FILES
    ${Installed_Data_Files}
    DESTINATION Data/${PLUGIN_NAME}
    COMPONENT Applications
  )
  install(DIRECTORY
    ${${PLUGIN_NAME}_SOURCE_DIR}/data/IPF_Legend
    DESTINATION Data/${PLUGIN_NAME}
    COMPONENT Applications
  )
endif()


