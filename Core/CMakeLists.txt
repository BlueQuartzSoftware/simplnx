include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "Core")
#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# Core/src/Core/Filters/ directory.
set(FilterList
  #ArrayCalculator # MISSING 1 or more Parameter Implementations
  CombineAttributeArrays
  CombineAttributeMatrices
  ConvertColorToGrayScale
  ConvertData
  CopyFeatureArrayToElementArray
  CopyObject
  #CreateAttributeMatrix # MISSING 1 or more Parameter Implementations
  #CreateDataArray # MISSING 1 or more Parameter Implementations
  CreateDataContainer
  CreateFeatureArrayFromElementArray
  CreateGeometry
  CreateGridMontage
  CreateStringArray
  CropVertexGeometry
  #DataContainerReader # MISSING 1 or more Parameter Implementations
  DataContainerWriter
  ExecuteProcess
  ExtractAttributeArraysFromGeometry
  ExtractComponentAsArray
  ExtractVertexGeometry
  FeatureCountDecision
  FeatureDataCSVWriter
  FindDerivatives
  #GenerateColorTable # MISSING 1 or more Parameter Implementations
  GenerateTiltSeries
  GenerateVertexCoordinates
  ImportAsciDataArray
  #ImportHDF5Dataset # MISSING 1 or more Parameter Implementations
  #InitializeData # MISSING 1 or more Parameter Implementations
  LinkFeatureMapToElementArray
  MaskCountDecision
  MoveData
  #MoveMultiData # MISSING 1 or more Parameter Implementations
  #MultiThresholdObjects # MISSING 1 or more Parameter Implementations
  #MultiThresholdObjects2 # MISSING 1 or more Parameter Implementations
  #PipelineAnnotation # MISSING 1 or more Parameter Implementations
  PostSlackMessage
  RawBinaryReader
  #ReadASCIIData # MISSING 1 or more Parameter Implementations
  #RemoveArrays # MISSING 1 or more Parameter Implementations
  RemoveComponentFromArray
  RenameAttributeArray
  RenameAttributeMatrix
  RenameDataContainer
  ReplaceValueInArray
  RequiredZThickness
  #RotateSampleRefFrame # MISSING 1 or more Parameter Implementations
  ScaleVolume
  SetOriginResolutionImageGeom
  SplitAttributeArray
  WriteASCIIData
  WriteTriangleGeometry

)

create_complex_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      DESCRIPTION "Plugin for ${PLUGIN_NAME}"
                      VERSION "0.1.0"
)

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME} PUBLIC [name of library])

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
# target_include_directories(${PLUGIN_NAME}
#     PUBLIC
#     additional include directories here
# )

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
