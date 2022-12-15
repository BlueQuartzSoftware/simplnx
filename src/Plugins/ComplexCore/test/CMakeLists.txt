# ------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  ArrayCalculatorTest.cpp
  AlignGeometriesTest.cpp
  ApproximatePointCloudHullTest.cpp
  ApplyTransformationToGeometryFilterTest.cpp
  CalculateTriangleAreasFilterTest.cpp
  CalculateFeatureSizesTest.cpp
  ChangeAngleRepresentationTest.cpp
  CombineAttributeArraysTest.cpp
  ConditionalSetValueTest.cpp
  ConvertColorToGrayScaleTest.cpp
  ConvertDataTest.cpp
  CopyDataObjectTest.cpp
  CreateAttributeMatrixTest.cpp
  CreateDataArrayTest.cpp
  CreateFeatureArrayFromElementArrayTest.cpp
  CreateGeometryTest.cpp
  CropVertexGeometryTest.cpp
  CopyFeatureArrayToElementArrayTest.cpp
  CreateImageGeometryTest.cpp
  CropImageGeometryTest.cpp
  ExtractInternalSurfacesFromTriangleGeometryTest.cpp
  ExtractVertexGeometryTest.cpp
  FeatureDataCSVWriterTest.cpp
  FindDifferencesMapTest.cpp
  FindFeatureCentroidsFilterTest.cpp
  FindFeaturePhasesFilterTest.cpp
  FindNeighborListStatisticsTest.cpp
  FindNeighborsTest.cpp
  FindSurfaceFeaturesTest.cpp
  FindTriangleGeomCentroidsTest.cpp
  FindTriangleGeomSizesTest.cpp
  IdentifySampleTest.cpp
  ImageGeomTest.cpp
  IterativeClosestPointTest.cpp
  InitializeDataTest.cpp
  InterpolatePointCloudToRegularGridTest.cpp
  ImportHDF5DatasetTest.cpp
  LaplacianSmoothingFilterTest.cpp
  MapPointCloudToRegularGridTest.cpp
  MinNeighborsTest.cpp
  MoveDataTest.cpp
  PointSampleTriangleGeometryFilterTest.cpp
  QuickSurfaceMeshFilterTest.cpp
  ImportCSVDataTest.cpp
  RemoveFlaggedVerticesTest.cpp
  RemoveMinimumSizeFeaturesTest.cpp
  RenameDataObjectTest.cpp
  RobustAutomaticThresholdTest.cpp
  RawBinaryReaderTest.cpp
  ScalarSegmentFeaturesFilterTest.cpp
  SetImageGeomOriginScalingFilterTest.cpp
  SharedFeatureFaceTest.cpp
  StlFileReaderTest.cpp
  FindArrayStatisticsTest.cpp
  TriangleNormalFilterTest.cpp
  TriangleDihedralAngleFilterTest.cpp
  FindNumFeaturesTest.cpp
  FindVolFractionsTest.cpp
  ExtractComponentAsArrayTest.cpp
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
# PUBLIC
# additional header files
# PRIVATE
# additional source files
# )

# ------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
target_include_directories(${PLUGIN_NAME}UnitTest
  PUBLIC
  ${complex_SOURCE_DIR}/test
)
