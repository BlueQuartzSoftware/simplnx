
#------------------------------------------------------------------------------
# Required EbsdLib and H5Support
#------------------------------------------------------------------------------
find_package (EbsdLib REQUIRED)

#------------------------------------------------------------------------------
# Include the standard complex plugin cmake support codes
#------------------------------------------------------------------------------
include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "OrientationAnalysis")
#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# OrientationAnalysis/src/OrientationAnalysis/Filters/ directory.
set(FilterList
  BadDataNeighborOrientationCheck

  #ConvertHexGridToSquareGrid # MISSING 1 or more Parameter Implementations
  ConvertOrientations
  ConvertQuaternion
  #CreateEnsembleInfo # MISSING 1 or more Parameter Implementations
  CreateLambertSphere
  EMsoftSO3Sampler
  #EbsdToH5Ebsd # MISSING 1 or more Parameter Implementations
  EnsembleInfoReader
  FindAvgCAxes
  FindAvgOrientations
  FindBoundaryStrengths
  FindCAxisLocations
  FindDistsToCharactGBs
  FindFeatureNeighborCAxisMisalignments
  FindFeatureReferenceCAxisMisorientations
  FindFeatureReferenceMisorientations
  FindGBCD
  FindGBCDMetricBased
  FindGBPDMetricBased
  FindKernelAvgMisorientations
  FindMisorientations
  FindSchmids
  FindSlipTransmissionMetrics
  FindTwinBoundaries
  FindTwinBoundarySchmidFactors
  GenerateFZQuaternions
  GenerateFaceIPFColoring
  GenerateFaceMisorientationColoring
  GenerateIPFColors
  GenerateOrientationMatrixTranspose
  GenerateQuaternionConjugate
  INLWriter
  #ImportEbsdMontage # MISSING 1 or more Parameter Implementations
  #ImportH5EspritData # MISSING 1 or more Parameter Implementations
  #ImportH5OimData # MISSING 1 or more Parameter Implementations
  NeighborOrientationCorrelation
  #OrientationUtility # MISSING 1 or more Parameter Implementations
  ReadAngData
  ReadCtfData
  #ReadH5Ebsd # MISSING 1 or more Parameter Implementations
  ReplaceElementAttributesWithNeighborValues
  RodriguesConvertor
  RotateEulerRefFrame
  Stereographic3D
  WritePoleFigure
  WriteStatsGenOdfAngleFile
)

create_complex_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      DESCRIPTION "Plugin for ${PLUGIN_NAME}"
                      VERSION "0.1.0"
)

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
find_package(H5Support REQUIRED)
find_package(EbsdLib REQUIRED)


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
target_link_libraries(${PLUGIN_NAME} PUBLIC EbsdLib::EbsdLib)

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}
#   PUBLIC
#     additional header files
#   PRIVATE
#     additional source files
# )

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
 target_include_directories(${PLUGIN_NAME}
     PUBLIC
     ${EbsdLib_INCLUDE_DIRS}
 )

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
