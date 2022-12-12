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
  CopyDataObjectTest.cpp
  CreateAttributeMatrixTest.cpp
  CreateDataArrayTest.cpp
  CreateFeatureArrayFromElementArrayTest.cpp
  CropVertexGeometryTest.cpp
  CopyFeatureArrayToElementArrayTest.cpp
  CreateImageGeometryTest.cpp
  CropImageGeometryTest.cpp
  ExtractInternalSurfacesFromTriangleGeometryTest.cpp
  ExtractVertexGeometryTest.cpp
  FindDifferencesMapTest.cpp
  FindFeaturePhasesFilterTest.cpp
  FindNeighborListStatisticsTest.cpp
  FindNeighborsTest.cpp
  FindSurfaceFeaturesTest.cpp
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
  PartitionGeometryTest.cpp
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
  StlFileReaderTest.cpp
  FindArrayStatisticsTest.cpp
  TriangleNormalFilterTest.cpp
  TriangleDihedralAngleFilterTest.cpp
  FindNumFeaturesTest.cpp
  FindVolFractionsTest.cpp
  ExtractComponentAsArrayTest.cpp
  CopyDataGroupTest.cpp
  CropImageGeomTest.cpp
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

# -----------------------------------------------------------------------------
# Here we are going to setup to download and decompress the test files. In order
# to add your own test files you will need to tar.gz the test file, compute the
# SHA 512 Hash of the file and then upload the file to 
# https://github.com/BlueQuartzSoftware/complex/releases/tag/Data_Archive.
#
# Go to the web site above, "edit" the release, add the filename and SHA 512 to
# the table and then upload your compressed file.
# Save the release so that the repo is updated
# -----------------------------------------------------------------------------

if(EXISTS "${DREAM3D_DATA_DIR}" AND COMPLEX_DOWNLOAD_TEST_FILES) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME PartitionGeometryTest.tar.gz SHA512 8074dbcff86ed206d9185065a56258b766eb24367b3481e8600e9b48542de23dbd715ef8f9eca1c9718ca5e05a9c48997f613bde6f4357b8bba206241caecbbd)

endif()
