# ------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  AlignGeometriesTest.cpp
  ApplyTransformationToGeometryFilterTest.cpp
  ApproximatePointCloudHullTest.cpp
  ArrayCalculatorTest.cpp
  CalculateFeatureSizesTest.cpp
  CalculateTriangleAreasFilterTest.cpp
  ChangeAngleRepresentationTest.cpp
  CombineAttributeArraysTest.cpp
  ConditionalSetValueTest.cpp
  ConvertColorToGrayScaleTest.cpp
  ConvertDataTest.cpp
  CopyDataObjectTest.cpp
  CopyFeatureArrayToElementArrayTest.cpp
  CreateAttributeMatrixTest.cpp
  CreateDataArrayTest.cpp
  CreateFeatureArrayFromElementArrayTest.cpp
<<<<<<< HEAD
  CreateGeometryTest.cpp
  CropVertexGeometryTest.cpp
  CopyFeatureArrayToElementArrayTest.cpp
=======
>>>>>>> 58f9fd5 (FILTER: TriangleCentroidFilter added with unit test.)
  CreateImageGeometryTest.cpp
  CropImageGeometryTest.cpp
  CropVertexGeometryTest.cpp
  ExtractComponentAsArrayTest.cpp
  ExtractInternalSurfacesFromTriangleGeometryTest.cpp
  ExtractVertexGeometryTest.cpp
<<<<<<< HEAD
  FeatureDataCSVWriterTest.cpp
=======
  FindArrayStatisticsTest.cpp
>>>>>>> 58f9fd5 (FILTER: TriangleCentroidFilter added with unit test.)
  FindDifferencesMapTest.cpp
  FindFeatureCentroidsFilterTest.cpp
  FindFeaturePhasesFilterTest.cpp
  FindNeighborListStatisticsTest.cpp
  FindNeighborsTest.cpp
  FindNumFeaturesTest.cpp
  FindSurfaceFeaturesTest.cpp
<<<<<<< HEAD
  FindTriangleGeomCentroidsTest.cpp
  FindTriangleGeomSizesTest.cpp
=======
  FindVolFractionsTest.cpp
>>>>>>> 58f9fd5 (FILTER: TriangleCentroidFilter added with unit test.)
  IdentifySampleTest.cpp
  ImageGeomTest.cpp
  ImportCSVDataTest.cpp
  ImportHDF5DatasetTest.cpp
  InitializeDataTest.cpp
  InterpolatePointCloudToRegularGridTest.cpp
  IterativeClosestPointTest.cpp
  LaplacianSmoothingFilterTest.cpp
  MapPointCloudToRegularGridTest.cpp
  MinNeighborsTest.cpp
  MoveDataTest.cpp
  PointSampleTriangleGeometryFilterTest.cpp
  QuickSurfaceMeshFilterTest.cpp
  RawBinaryReaderTest.cpp
  RemoveFlaggedVerticesTest.cpp
  RemoveMinimumSizeFeaturesTest.cpp
  RenameDataObjectTest.cpp
  RobustAutomaticThresholdTest.cpp
  ScalarSegmentFeaturesFilterTest.cpp
  SetImageGeomOriginScalingFilterTest.cpp
  SharedFeatureFaceTest.cpp
  StlFileReaderTest.cpp
  TriangleCentroidTest.cpp
  TriangleDihedralAngleFilterTest.cpp
<<<<<<< HEAD
  FindNumFeaturesTest.cpp
  FindVolFractionsTest.cpp
  ExtractComponentAsArrayTest.cpp
  WriteBinaryDataTest.cpp
=======
  TriangleNormalFilterTest.cpp
>>>>>>> 58f9fd5 (FILTER: TriangleCentroidFilter added with unit test.)
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
