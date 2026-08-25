include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "SimplnxCore")
set(${PLUGIN_NAME}_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

# ------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# PLUGIN_NAME/src/PLUGIN_NAME/Filters/ directory.
set(FilterList
  AddBadDataFilter
  AlignGeometriesFilter
  AlignSectionsFeatureCentroidFilter
  AlignSectionsListFilter
  AppendImageGeometryFilter
  ApplyTransformationToGeometryFilter
  ApproximatePointCloudHullFilter
  ArrayCalculatorFilter
  ChangeAngleRepresentationFilter
  CombineAttributeArraysFilter
  CombineNodeBasedGeometriesFilter
  CombineStlFilesFilter
  CombineTransformationMatricesFilter
  ComputeArrayHistogramByFeatureFilter
  ComputeArrayHistogramFilter
  ComputeArrayStatisticsFilter
  ComputeBiasedFeaturesFilter
  ComputeBoundaryCellsFilter
  ComputeBoundaryElementFractionsFilter
  ComputeBoundingBoxStatsFilter
  ComputeCoordinatesImageGeomFilter
  ComputeCoordinateThresholdFilter
  ComputeDifferencesMapFilter
  ComputeEuclideanDistMapFilter
  ComputeFeatureBoundsFilter
  ComputeFeatureCentroidsFilter
  ComputeFeatureClusteringFilter
  ComputeFeatureNeighborsFilter
  ComputeFeaturePhasesBinaryFilter
  ComputeFeaturePhasesFilter
  ComputeFeatureRectFilter
  ComputeFeatureSizesFilter
  ComputeGroupingDensityFilter
  ComputeKMeansFilter
  ComputeKMedoidsFilter
  ComputeLargestCrossSectionsFilter
  ComputeMomentInvariants2DFilter
  ComputeNeighborhoodsFilter
  ComputeNeighborListStatisticsFilter
  ComputeNumFeaturesFilter
  ComputeSurfaceAreaToVolumeFilter
  ComputeSurfaceFeaturesFilter
  ComputeTriangleAreasFilter
  ComputeTriangleGeomCentroidsFilter
  ComputeTriangleGeomVolumesFilter
  ComputeVectorColorsFilter
  ComputeVertexToTriangleDistancesFilter
  ComputeVolumeFractionsFilter
  ConcatenateDataArraysFilter
  ConditionalSetValueFilter
  ConvertColorToGrayScaleFilter
  ConvertDataFilter
  CopyDataObjectFilter
  CopyFeatureArrayToElementArrayFilter
  CreateAMScanPathsFilter
  CreateAttributeMatrixFilter
  CreateColorMapFilter
  CreateDataArrayAdvancedFilter
  CreateDataArrayFilter
  CreateDataGroupFilter
  CreateFeatureArrayFromElementArrayFilter
  CreateGeometryFilter
  CreateImageGeometryFilter
  CreatePythonSkeletonFilter
  CropEdgeGeometryFilter
  CropImageGeometryFilter
  CropVertexGeometryFilter
  DBSCANFilter
  DeleteDataFilter
  ErodeDilateBadDataFilter
  ErodeDilateCoordinationNumberFilter
  ErodeDilateMaskFilter
  ExecuteProcessFilter
  ExtractComponentAsArrayFilter
  ExtractInternalSurfacesFromTriangleGeometryFilter
  ExtractPipelineToFileFilter
  ExtractVertexGeometryFilter
  FeatureFaceCurvatureFilter
  FillBadDataFilter
  FlyingEdges3DFilter
  HierarchicalSmoothFilter
  IdentifyDuplicateVerticesFilter
  IdentifySampleFilter
  InitializeDataFilter
  InitializeImageGeomCellDataFilter
  InterpolatePointCloudToRegularGridFilter
  IterativeClosestPointFilter
  KeepRemoveRankedFeaturesFilter
  LabelTriangleGeometryFilter
  LaplacianSmoothingFilter
  M3CSurfaceMeshingFilter
  MapPointCloudToRegularGridFilter
  MoveDataFilter
  MultiThresholdObjectsFilter
  NearestPointFuseRegularGridsFilter
  PadImageGeometryFilter
  PartitionGeometryFilter
  ExtractFeatureBoundaries2DFilter
  ExtractTripleLinesFilter
  PointSampleEdgeGeometryFilter
  PointSampleTriangleGeometryFilter
  QuickSurfaceMeshFilter
  RandomizeFeatureIdsFilter
  ReadBinaryCTNorthstarFilter
  ReadCSVFileFilter
  ReadDeformKeyFileV12Filter
  ReadDREAM3DFilter
  ReadHDF5DatasetFilter
  ReadImageFilter
  ReadImageStackFilter
  ReadNIfTIFileFilter
  ReadRawBinaryFilter
  ReadStlFileFilter
  ReadStringDataArrayFilter
  ReadTextDataArrayFilter
  ReadVolumeGraphicsFileFilter
  ReadVtkStructuredPointsFilter
  ReadZeissTxmFileFilter
  RegularGridSampleSurfaceMeshFilter
  RegularizeZSpacingFilter
  RemoveFlaggedEdgesFilter
  RemoveFlaggedFeaturesFilter
  RemoveFlaggedTrianglesFilter
  RemoveFlaggedVerticesFilter
  RenameDataObjectFilter
  ReplaceElementAttributesWithNeighborValuesFilter
  RequireMinimumSizeFeaturesFilter
  RequireMinNumNeighborsFilter
  ResampleImageGeomFilter
  ResampleRectGridToImageGeomFilter
  ReshapeDataArrayFilter
  ReverseTriangleWindingFilter
  RobustAutomaticThresholdFilter
  RotateSampleRefFrameFilter
  ScalarSegmentFeaturesFilter
  SetImageGeomOriginScalingFilter
  SharedFeatureFaceFilter
  SilhouetteFilter
  SliceTriangleGeometryFilter
  SplitDataArrayByComponentFilter
  SplitDataArrayByTupleFilter
  SurfaceNetsFilter
  TriangleCentroidFilter
  TriangleDihedralAngleFilter
  TriangleNormalFilter
  UncertainRegularGridSampleSurfaceMeshFilter
  VerifyTriangleWindingFilter
  WriteAbaqusHexahedronFilter
  WriteASCIIDataFilter
  WriteAvizoRectilinearCoordinateFilter
  WriteAvizoUniformCoordinateFilter
  WriteBinaryDataFilter
  WriteDREAM3DFilter
  WriteFeatureDataCSVFilter
  WriteLAMMPSFileFilter
  WriteLosAlamosFFTFilter
  WriteNodesAndElementsFilesFilter
  WriteSPParksSitesFilter
  WriteStlFileFilter
  WriteVtkRectilinearGridFilter
  WriteVtkStructuredPointsFilter
  WriteImageFilter
)

set(ActionList
)

set(AlgorithmList
  AlignGeometries
  AddBadData
  AlignSectionsFeatureCentroid
  AlignSectionsList
  AppendImageGeometry
  ApplyTransformationToGeometry
  ApproximatePointCloudHull
  ArrayCalculator
  CalculateTriangleGroupCurvatures
  ChangeAngleRepresentation
  CombineAttributeArrays
  CombineNodeBasedGeometries
  CombineStlFiles
  CombineTransformationMatrices
  ComputeArrayHistogram
  ComputeArrayHistogramByFeature
  ComputeArrayStatistics
  ComputeBiasedFeatures
  ComputeBoundaryCells
  ComputeBoundingBoxStats
  ComputeCoordinatesImageGeom
  ComputeCoordinateThreshold
  ComputeBoundaryElementFractions
  ComputeDifferencesMap
  ComputeEuclideanDistMap
  ComputeFeatureBounds
  ComputeFeatureCentroids
  ComputeFeatureClustering
  ComputeFeatureNeighbors
  ComputeFeaturePhases
  ComputeFeaturePhasesBinary
  ComputeFeatureRect
  ComputeFeatureSizes
  ComputeGroupingDensity
  ComputeKMeans
  ComputeKMedoids
  ComputeLargestCrossSections
  ComputeMomentInvariants2D
  ComputeNeighborhoods
  ComputeNeighborListStatistics
#  ComputeNumFeatures
  ComputeSurfaceAreaToVolume
  ComputeSurfaceFeatures
#  ComputeTriangleAreas
  ComputeTriangleGeomCentroids
  ComputeTriangleGeomVolumes
  ComputeVectorColors
  ComputeVertexToTriangleDistances
#  ComputeVolumeFractions
  ConcatenateDataArrays
  ConditionalSetValue
  ConvertColorToGrayScale
  ConvertData
 # CopyDataObject
  CopyFeatureArrayToElementArray
  CreateAMScanPaths
 # CreateAttributeMatrix
  CreateColorMap
 # CreateDataArray
#  CreateDataArrayAdvanced
#  CreateDataGroup
  CreateFeatureArrayFromElementArray
  CreateGeometry
#  CreateImageGeometry
  CreatePythonSkeleton
  CropEdgeGeometry
  CropImageGeometry
  CropVertexGeometry
  DBSCAN
 # DeleteData
  ErodeDilateBadData
  ErodeDilateCoordinationNumber
  ErodeDilateMask
  ExecuteProcess
  ExtractComponentAsArray
  ExtractInternalSurfacesFromTriangleGeometry
  ExtractPipelineToFile
  ExtractVertexGeometry
  FeatureFaceCurvature
  FillBadData
  FindNRingNeighbors
  FlyingEdges3D
  HierarchicalSmooth
  IdentifyDuplicateVertices
  IdentifySample
  InitializeData
  InitializeImageGeomCellData
  InterpolatePointCloudToRegularGrid
  IterativeClosestPoint
  InterpolatePointCloudToRegularGrid
  KeepRemoveRankedFeatures
  LabelTriangleGeometry
  LaplacianSmoothing
  M3CSurfaceMeshing
  MapPointCloudToRegularGrid
#  MoveData
  MultiThresholdObjects
  NearestPointFuseRegularGrids
  PadImageGeometry
  PartitionGeometry
  PointSampleEdgeGeometry
  ExtractFeatureBoundaries2D
  PointSampleTriangleGeometry
  QuickSurfaceMesh
  ReadBinaryCTNorthstar
  ReadCSVFile
  ReadDeformKeyFileV12
#  ReadDREAM3D
  ReadHDF5Dataset
  ReadImage
  ReadImageStack
  ReadNIfTIFile
  ReadRawBinary
  ReadStlFile
  ReadStringDataArray
  ReadTextDataArray
  ReadVolumeGraphicsFile
  ReadVtkStructuredPoints
  ReadZeissTxmFile
  RegularGridSampleSurfaceMesh
  RegularizeZSpacing
  RemoveFlaggedEdges
  RemoveFlaggedFeatures
  RemoveFlaggedTriangles
  RemoveFlaggedVertices
 # RenameDataObject
  ReplaceElementAttributesWithNeighborValues
  RequireMinNumNeighbors
  RequireMinimumSizeFeatures
  ResampleImageGeom
  ResampleRectGridToImageGeom
  ReshapeDataArray
  ReverseTriangleWinding
  RobustAutomaticThreshold
  RotateSampleRefFrame
  ScalarSegmentFeatures
 # SetImageGeomOriginScaling
  SharedFeatureFace
  Silhouette
  SliceTriangleGeometry
  SplitDataArrayByComponent
  SplitDataArrayByTuple
  SurfaceNets
  TriangleCentroid
  TriangleDihedralAngle
  TriangleNormal
  TupleTransfer
  UncertainRegularGridSampleSurfaceMesh
  VerifyTriangleWinding
  WriteAbaqusHexahedron
  WriteASCIIData
  WriteAvizoRectilinearCoordinate
  WriteAvizoUniformCoordinate
  WriteLAMMPSFile
  WriteBinaryData
  WriteDREAM3D
  WriteFeatureDataCSV
  WriteLosAlamosFFT
  WriteNodesAndElementsFiles
  WriteSPParksSites
  WriteStlFile
  WriteVtkRectilinearGrid
  WriteVtkStructuredPoints
  WriteImage
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


# ------------------------------------------------------------------------------
# The ReadNIfTIFileFilter requires zlib for transparent .nii/.nii.gz support
# ------------------------------------------------------------------------------
find_package(ZLIB REQUIRED)

#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
target_link_libraries(${PLUGIN_NAME} PRIVATE reproc++ ZLIB::ZLIB)

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call

set(PLUGIN_EXTRA_SOURCES
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/nanoflann.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/StlUtilities.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/StlUtilities.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/AvizoWriter.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/AvizoWriter.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginTemplateFile.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/VtkUtilities.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/nifti1.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NiftiUtilities.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/NiftiUtilities.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/FeatureRemovalUtilities.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/FeatureRemovalUtilities.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellFlag.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellFlag.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellMap.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMCellMap.h"
    # "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryGL.cpp"
    # "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryGL.h"
    # "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryOBJ.cpp"
    # "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMGeometryOBJ.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMSurfaceNet.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/SurfaceNets/MMSurfaceNet.h"

)

set(OLESS_SOURCES 
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/oless_common.hpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/oless.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/oless.h"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/pole.cpp"
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless/oless/pole.h"
)

target_sources(${PLUGIN_NAME}
  PRIVATE
    ${PLUGIN_EXTRA_SOURCES} ${OLESS_SOURCES}
)

source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" 
            PREFIX ${PLUGIN_NAME}
            FILES  ${PLUGIN_EXTRA_SOURCES}
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

file(READ "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginEnvironmentTemplate.py" PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE)
string(HEX ${PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE} PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE ${PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE})
string(APPEND PYTHON_PLUGIN_ENVIRONMENT_TEMPLATE "0x00")

file(READ "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginPyProjectTemplate.py" PYTHON_PLUGIN_PYPROJECT_TEMPLATE)
string(HEX ${PYTHON_PLUGIN_PYPROJECT_TEMPLATE} PYTHON_PLUGIN_PYPROJECT_TEMPLATE)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," PYTHON_PLUGIN_PYPROJECT_TEMPLATE ${PYTHON_PLUGIN_PYPROJECT_TEMPLATE})
string(APPEND PYTHON_PLUGIN_PYPROJECT_TEMPLATE "0x00")

file(READ "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginMetaTemplate.py" PYTHON_PLUGIN_META_TEMPLATE)
string(HEX ${PYTHON_PLUGIN_META_TEMPLATE} PYTHON_PLUGIN_META_TEMPLATE)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\0," PYTHON_PLUGIN_META_TEMPLATE ${PYTHON_PLUGIN_META_TEMPLATE})
string(APPEND PYTHON_PLUGIN_META_TEMPLATE "0x00")

cmpConfigureFileWithMD5Check(CONFIGURED_TEMPLATE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/utils/PythonPluginSourceTemplate.in.hpp"
                               GENERATED_FILE_PATH "${${PLUGIN_NAME}_BINARY_DIR}/generated/${PLUGIN_NAME}/utils/PythonPluginSourceTemplate.hpp")

                               #------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
target_include_directories(${PLUGIN_NAME}
  PUBLIC
    "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/oless"
)

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
  set(PIPELINE_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/pipelines")
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
  # add_custom_target(Copy_${PLUGIN_NAME}_ASCIIData ALL
  #   COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/ASCIIData.tar.gz"
  #   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/ASCIIData" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data/ASCIIData"
  #   COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/ASCIIData"
  #   WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
  #   COMMENT "Copying ${PLUGIN_NAME}/ASCIIData data into Binary Directory"
  #   DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
  #   COMMAND_EXPAND_LISTS
  #   VERBATIM
  # )
  # set_target_properties(Copy_${PLUGIN_NAME}_ASCIIData PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME STL_Models.tar.gz
    SHA512 6a17caa1c5b60740a18d30732b10139b5864e12abd5009ce482972f4070f86e1762a3bf2bbceb8f178187b4d70bedbde1516f32397c3d4424404557b320dde9d
    INSTALL
    COPY_DATA
  )
  # add_custom_target(Copy_${PLUGIN_NAME}_STL_Models ALL
  #   COMMAND ${CMAKE_COMMAND} -E tar xzf "${DREAM3D_DATA_DIR}/TestFiles/STL_Models.tar.gz"
  #   COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different "${DREAM3D_DATA_DIR}/TestFiles/STL_Models" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/Data/STL_Models"
  #   COMMAND ${CMAKE_COMMAND} -E rm -rf "${DREAM3D_DATA_DIR}/TestFiles/STL_Models"
  #   WORKING_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles"
  #   COMMENT "Copying ${PLUGIN_NAME}/STL_Models data into Binary Directory"
  #   DEPENDS Fetch_Remote_Data_Files  # Make sure all remote files are downloaded before trying this
  #   COMMAND_EXPAND_LISTS
  #   VERBATIM
  # )
  # set_target_properties(Copy_${PLUGIN_NAME}_STL_Models PROPERTIES FOLDER Plugins/${PLUGIN_NAME})

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME align_sections_input.tar.gz
    SHA512 3c12c306fc8bba6fcf31a4c2c0c1771a5320267e02d12d7c74897852b73b7b6d74c5414aad9a90f64090e6b9baa6968925ea322ea0c3cb10fbb6022326c74e0b
    INSTALL
    COPY_DATA
  )

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR}
    ARCHIVE_NAME dbscan_2d_data.tar.gz
    SHA512 cc887291d92f6733de88019249624fcba2889f7594f988fcf083b642b4b2c55321f870c8f8be61a84e982db752e6e385ade930b80b49d718435c827530f479a6
    INSTALL
    COPY_DATA
  )

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
