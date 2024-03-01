include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "SimplnxCore")
set(${PLUGIN_NAME}_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# PLUGIN_NAME/src/PLUGIN_NAME/Filters/ directory.
set(FilterList
  AddBadDataFilter
  AlignGeometries
  AlignSectionsFeatureCentroidFilter
  AlignSectionsListFilter
  AppendImageGeometryZSliceFilter
  ApplyTransformationToGeometryFilter
  ApproximatePointCloudHull
  ArrayCalculatorFilter
  CalculateArrayHistogramFilter
  CalculateFeatureSizesFilter
  CalculateTriangleAreasFilter
  ChangeAngleRepresentation
  CombineAttributeArraysFilter
  CombineStlFilesFilter
  ComputeFeatureRectFilter
  ComputeMomentInvariants2DFilter
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
  ErodeDilateCoordinationNumberFilter
  ErodeDilateMaskFilter
  ExecuteProcessFilter
  ExtractComponentAsArrayFilter
  ExtractInternalSurfacesFromTriangleGeometry
  ExtractVertexGeometryFilter
  FeatureFaceCurvatureFilter
  FillBadDataFilter
  FindArrayStatisticsFilter
  FindBiasedFeaturesFilter
  FindBoundaryCellsFilter
  FindBoundaryElementFractionsFilter
  FindDifferencesMap
  FindEuclideanDistMapFilter
  FindFeatureCentroidsFilter
  FindFeatureClusteringFilter
  FindFeaturePhasesBinaryFilter
  FindFeaturePhasesFilter
  FindLargestCrossSectionsFilter
  FindNeighborhoodsFilter
  FindNeighborListStatistics
  FindNeighbors
  FindNumFeaturesFilter
  FindSurfaceAreaToVolumeFilter
  FindSurfaceFeatures
  FindTriangleGeomCentroidsFilter
  FindTriangleGeomSizesFilter
  FindVertexToTriangleDistancesFilter
  FindVolFractionsFilter
  FlyingEdges3DFilter
  GenerateColorTableFilter
  GeneratePythonSkeletonFilter
  GenerateVectorColorsFilter
  IdentifySample
  InitializeData
  InitializeImageGeomCellData
  InterpolatePointCloudToRegularGridFilter
  IterativeClosestPointFilter
  KMeansFilter
  KMedoidsFilter
  LabelTriangleGeometryFilter
  LaplacianSmoothingFilter
  MapPointCloudToRegularGridFilter
  MinNeighbors
  MoveData
  MultiThresholdObjects
  NearestPointFuseRegularGridsFilter
  PartitionGeometryFilter
  PointSampleTriangleGeometryFilter
  QuickSurfaceMeshFilter
  ReadBinaryCTNorthstarFilter
  ReadCSVFileFilter
  ReadDeformKeyFileV12Filter
  ReadDREAM3DFilter
  ReadHDF5Dataset
  ReadRawBinaryFilter
  ReadStlFileFilter
  ReadTextDataArrayFilter
  ReadVolumeGraphicsFileFilter
  RegularGridSampleSurfaceMeshFilter
  RemoveFlaggedFeaturesFilter
  RemoveFlaggedTrianglesFilter
  RemoveFlaggedVertices
  RemoveMinimumSizeFeaturesFilter
  RenameDataObject
  ReplaceElementAttributesWithNeighborValuesFilter
  ResampleImageGeomFilter
  ResampleRectGridToImageGeomFilter
  ReverseTriangleWindingFilter
  RobustAutomaticThreshold
  RotateSampleRefFrameFilter
  ScalarSegmentFeaturesFilter
  SetImageGeomOriginScalingFilter
  SharedFeatureFaceFilter
  SilhouetteFilter
  SplitAttributeArrayFilter
  SurfaceNetsFilter
  TriangleCentroidFilter
  TriangleDihedralAngleFilter
  TriangleNormalFilter
  UncertainRegularGridSampleSurfaceMeshFilter
  WriteAbaqusHexahedronFilter
  WriteASCIIDataFilter
  WriteAvizoRectilinearCoordinateFilter
  WriteAvizoUniformCoordinateFilter
  WriteBinaryDataFilter
  WriteDREAM3DFilter
  WriteFeatureDataCSVFilter
  WriteLosAlamosFFTFilter
  WriteStlFileFilter
  WriteVtkRectilinearGridFilter
  ReadVtkStructuredPointsFilter
)

set(ActionList
)

set(AlgorithmList
  AddBadData
  AlignSectionsFeatureCentroid
  AlignSectionsList
  AppendImageGeometryZSlice
  ApplyTransformationToGeometry
  ArrayCalculator
  CalculateArrayHistogram
  CalculateTriangleGroupCurvatures
  CombineAttributeArrays
  CombineStlFiles
  ComputeFeatureRect
  ComputeMomentInvariants2D
  ConvertColorToGrayScale
  ConvertData
  ErodeDilateBadData
  ErodeDilateCoordinationNumber
  ErodeDilateMask
  ExecuteProcess
  ExtractComponentAsArray
  ExtractVertexGeometry
  FeatureFaceCurvature
  FillBadData
  FindArrayStatistics
  FindBiasedFeatures
  FindBoundaryCells
  FindEuclideanDistMap
  FindFeatureCentroids
  FindFeatureClustering
  FindLargestCrossSections
  FindNeighborhoods
  FindNRingNeighbors
  FindSurfaceAreaToVolume
  FindTriangleGeomCentroids
  FindTriangleGeomSizes
  FindVertexToTriangleDistances
  FlyingEdges3D
  GenerateColorTable
  GeneratePythonSkeleton
  GenerateVectorColors
  KMeans
  KMedoids
  LabelTriangleGeometry
  LaplacianSmoothing
  NearestPointFuseRegularGrids
  PartitionGeometry
  PointSampleTriangleGeometry
  QuickSurfaceMesh
  ReadBinaryCTNorthstar
  ReadDeformKeyFileV12
  ReadRawBinary
  ReadStlFile
  ReadVolumeGraphicsFile
  RegularGridSampleSurfaceMesh
  RemoveFlaggedFeatures
  RemoveFlaggedTriangles
  ReplaceElementAttributesWithNeighborValues
  ResampleImageGeom
  ResampleRectGridToImageGeom
  ScalarSegmentFeatures
  SharedFeatureFace
  Silhouette
  SplitAttributeArray
  SurfaceNets
  TriangleCentroid
  TupleTransfer
  UncertainRegularGridSampleSurfaceMesh
  WriteAbaqusHexahedron
  WriteAvizoRectilinearCoordinate
  WriteAvizoUniformCoordinate
  WriteLosAlamosFFT
  WriteStlFile
  WriteVtkRectilinearGrid
  ReadVtkStructuredPoints
)

create_simplnx_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      ACTION_LIST ${ActionList}
                      ALGORITHM_LIST ${AlgorithmList}
                      DESCRIPTION "${PLUGIN_NAME} Plugin"
                      VERSION "0.1.0"
                      DOC_CHECK
                      ADD_TO_GLOBAL_LIST
)

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
if(SIMPLNX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
endif()

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, there is where
# to do that using the usual find_package(....) call
# ------------------------------------------------------------------------------
# The ExecuteProcess Filter requires the `reproc` library
# ------------------------------------------------------------------------------
find_package("reproc++" CONFIG REQUIRED)
get_target_property(reproc_dll_path reproc++ IMPORTED_LOCATION_RELEASE)
get_filename_component(reproc_dll_path "${reproc_dll_path}" DIRECTORY)
file(REAL_PATH "${reproc_dll_path}" reproc_dll_path)
file(TO_CMAKE_PATH "${reproc_dll_path}" reproc_dll_path)
get_property(SIMPLNX_EXTRA_LIBRARY_DIRS GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS)
set_property(GLOBAL PROPERTY SIMPLNX_EXTRA_LIBRARY_DIRS ${SIMPLNX_EXTRA_LIBRARY_DIRS} ${reproc_dll_path})


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
target_link_libraries(${PLUGIN_NAME} PRIVATE reproc++)

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call

set(PLUGIN_EXTRA_SOURCES
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/CSVDataParser.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/nanoflann.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/StlUtilities.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/StlUtilities.cpp"
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
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/AvizoWriter.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/AvizoWriter.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginTemplateFile.hpp"

    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellFlag.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellFlag.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellMap.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellMap.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryGL.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryGL.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryOBJ.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryOBJ.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMSurfaceNet.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMSurfaceNet.h"

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
# Configure the Python Plugin Generation Header file
# This will read the template files into the CMake variables, then configure
# the template file to use the contents of those files. This should allow easier
# updating of those template files.
file(READ "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonFilterTemplate.py" PYTHON_FILTER_TEMPLATE)
string(HEX ${PYTHON_FILTER_TEMPLATE} PYTHON_FILTER_TEMPLATE)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," PYTHON_FILTER_TEMPLATE ${PYTHON_FILTER_TEMPLATE})
string(APPEND PYTHON_FILTER_TEMPLATE "0x00")


file(READ "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginInitTemplate.py" PYTHON_PLUGIN_INIT_TEMPLATE)
string(HEX ${PYTHON_PLUGIN_INIT_TEMPLATE} PYTHON_PLUGIN_INIT_TEMPLATE)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," PYTHON_PLUGIN_INIT_TEMPLATE ${PYTHON_PLUGIN_INIT_TEMPLATE})
string(APPEND PYTHON_PLUGIN_INIT_TEMPLATE "0x00")

file(READ "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginTemplate.py" PYTHON_PLUGIN_TEMPLATE)
string(HEX ${PYTHON_PLUGIN_TEMPLATE} PYTHON_PLUGIN_TEMPLATE)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," PYTHON_PLUGIN_TEMPLATE ${PYTHON_PLUGIN_TEMPLATE})
string(APPEND PYTHON_PLUGIN_TEMPLATE "0x00")


cmpConfigureFileWithMD5Check(CONFIGURED_TEMPLATE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginSourceTemplate.in.hpp"
                              GENERATED_FILE_PATH "${${PLUGIN_NAME}_BINARY_DIR}/generated/${PLUGIN_NAME}/utils/PythonPluginSourceTemplate.hpp")

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}
#     PRIVATE
#     additional include directories here
# )

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
    set(INSTALL_DESTINATION "${SIMPLNX_PIPELINE_INSTALL_DIR}/${PLUGIN_NAME}")
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
    ARCHIVE_NAME ASCIIData.tar.gz
    SHA512 70388864301ca1ea7fce7b1666d3abf682eee68c7d8b7a9bf532df7aff11e7ea9de7dc2dc80e33f0e363cbad023b663bff97df4be362f0312d311e9d5bedf370
    INSTALL
    COPY_DATA
  )
  add_custom_target(Copy_${PLUGIN_NAME}_ASCIIData ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/ASCIIData.tar.gz"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/ASCIIData" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data/ASCIIData"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/ASCIIData"
    WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
    COMMENT "Copying ${PLUGIN_NAME}/ASCIIData data into Binary Directory"
    DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_ASCIIData PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME STL_Models.tar.gz
    SHA512 6a17caa1c5b60740a18d30732b10139b5864e12abd5009ce482972f4070f86e1762a3bf2bbceb8f178187b4d70bedbde1516f32397c3d4424404557b320dde9d
    INSTALL
    COPY_DATA
  )
  add_custom_target(Copy_${PLUGIN_NAME}_STL_Models ALL
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/STL_Models.tar.gz"
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/STL_Models" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data/STL_Models"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/STL_Models"
    WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
    COMMENT "Copying ${PLUGIN_NAME}/STL_Models data into Binary Directory"
    DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
    COMMAND_EXPAND_LISTS
    VERBATIM
  )
  set_target_properties(Copy_${PLUGIN_NAME}_STL_Models PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

endif()

# -----------------------------------------------------------------------
# Create build folder copy rules and install rules for the 'data' folder
# for this plugin
# -----------------------------------------------------------------------
set(Installed_Data_Files
)

install(FILES 
  ${Installed_Data_Files}
  DESTINATION Data/${PLUGIN_NAME}
  COMPONENT Applications
)
