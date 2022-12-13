include("${complex_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "ComplexCore")
set(${PLUGIN_NAME}_SOURCE_DIR ${complex_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# PLUGIN_NAME/src/PLUGIN_NAME/Filters/ directory.
set(FilterList
  AlignGeometries
  ApplyTransformationToGeometryFilter
  ApproximatePointCloudHull
  ArrayCalculatorFilter
  CalculateFeatureSizesFilter
  CalculateTriangleAreasFilter
  ChangeAngleRepresentation
  CombineAttributeArraysFilter
  ConditionalSetValue
  ConvertColorToGrayScaleFilter
  ConvertDataFilter
  CopyDataObjectFilter
  CopyFeatureArrayToElementArray
  CreateAttributeMatrixFilter
  CreateDataArray
  CreateDataGroup
  CreateFeatureArrayFromElementArray
  CreateGeometryFilter
  CreateImageGeometry
  CropImageGeometry
  CropVertexGeometry
  DeleteData
  ErodeDilateBadDataFilter
  ExportDREAM3DFilter
  ExtractInternalSurfacesFromTriangleGeometry
  ExtractVertexGeometryFilter
  FeatureDataCSVWriterFilter
  FindArrayStatisticsFilter
  FindDifferencesMap
  FindFeatureCentroidsFilter
  FindFeaturePhasesFilter
  FindNeighborListStatistics
  FindNeighbors
  FindSurfaceFeatures
  FindTriangleGeomCentroidsFilter
  FindTriangleGeomSizesFilter
  IdentifySample
  ImportCSVDataFilter
  ImportDREAM3DFilter
  ImportHDF5Dataset
  ImportTextFilter
  InitializeData
  InterpolatePointCloudToRegularGridFilter
  IterativeClosestPointFilter
  LaplacianSmoothingFilter
  MapPointCloudToRegularGridFilter
  MinNeighbors
  MoveData
  MultiThresholdObjects
  PointSampleTriangleGeometryFilter
  QuickSurfaceMeshFilter
  RawBinaryReaderFilter
  RemoveFlaggedVertices
  RemoveMinimumSizeFeaturesFilter
  RenameDataObject
  RobustAutomaticThreshold
  ScalarSegmentFeaturesFilter
  SetImageGeomOriginScalingFilter
  SharedFeatureFaceFilter
  StlFileReaderFilter
  TriangleDihedralAngleFilter
  TriangleNormalFilter
  FindNumFeaturesFilter
  FindVolFractionsFilter
  ExtractComponentAsArrayFilter
  WriteBinaryDataFilter
)

set(ActionList
)

set(AlgorithmList
  ApplyTransformationToGeometry
  ArrayCalculator
  ConvertColorToGrayScale
  ConvertData
  StlFileReader
  LaplacianSmoothing
  QuickSurfaceMesh
  PointSampleTriangleGeometry
  TupleTransfer
  RawBinaryReader
  ScalarSegmentFeatures
  FindFeatureCentroids
  FindArrayStatistics
  CombineAttributeArrays
  ErodeDilateBadData
  ExtractComponentAsArray
  ExtractVertexGeometry
  FindTriangleGeomCentroids
  FindTriangleGeomSizes
  SharedFeatureFace
)

create_complex_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      ACTION_LIST ${ActionList}
                      ALGORITHM_LIST ${AlgorithmList}
                      DESCRIPTION "Plugin for ${PLUGIN_NAME}"
                      VERSION "0.1.0"
                      DOC_CHECK
                      ADD_TO_GLOBAL_LIST
)

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
enable_testing()
add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, there is where
# to do that using the usual find_package(....) call
# find_package(....)


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME} PUBLIC [name of library])

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call

set(PLUGIN_EXTRA_SOURCES
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CSVDataParser.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/nanoflann.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/StlUtilities.hpp
    ${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/StlUtilities.cpp
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorItem.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorItem.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ICalculatorArray.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ICalculatorArray.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorArray.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/UnaryOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/UnaryOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/BinaryOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/BinaryOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ABSOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ABSOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PowOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PowOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ExpOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ExpOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/SinOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/SinOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CosOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CosOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/TanOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/TanOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ASinOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ASinOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ACosOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ACosOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ATanOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/ATanOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/SqrtOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/SqrtOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/RootOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/RootOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/LogOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/LogOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/Log10Operator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/Log10Operator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/LnOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/LnOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/AdditionOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/AdditionOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/SubtractionOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/SubtractionOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NegativeOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NegativeOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/MultiplicationOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/MultiplicationOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/DivisionOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/DivisionOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/LeftParenthesisItem.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/LeftParenthesisItem.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/RightParenthesisItem.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/RightParenthesisItem.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorSeparator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CalculatorSeparator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CommaSeparator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CommaSeparator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/FloorOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/FloorOperator.cpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CeilOperator.hpp"
	"${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CeilOperator.cpp"
)

target_sources(${PLUGIN_NAME}
  PRIVATE
    ${PLUGIN_EXTRA_SOURCES}
)

source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME}
  FILES
    ${PLUGIN_EXTRA_SOURCES}
)

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}
#     PUBLIC
#     additional include directories here
# )

# -----------------------------------------------------------------------
# Install example pipelines
# -----------------------------------------------------------------------
if(EXISTS "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines")
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CMAKE_CFG_INTDIR}/pipelines/${PLUGIN_NAME}")
  add_custom_target(${PLUGIN_NAME}PipelineFolderCopy ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines"
    ${PIPELINE_DEST_DIR}
    COMMENT "Copying Pipeline Folder into Binary Directory")
  set_target_properties(${PLUGIN_NAME}PipelineFolderCopy PROPERTIES FOLDER ZZ_COPY_FILES)

  set(INSTALL_DESTINATION "Pipelines/${PLUGIN_NAME}")
  install(DIRECTORY
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/"
    DESTINATION "${INSTALL_DESTINATION}"
    COMPONENT Applications
  )
endif()