# ------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  AbaqusHexahedronWriterTest.cpp
  AddBadDataTest.cpp
  AlignGeometriesTest.cpp
  AlignSectionsFeatureCentroidTest.cpp
  AlignSectionsListTest.cpp
  AppendImageGeometryZSliceTest.cpp
  ApplyTransformationToGeometryTest.cpp
  ApproximatePointCloudHullTest.cpp
  ArrayCalculatorTest.cpp
  CalculateArrayHistogramTest.cpp
  CalculateFeatureSizesTest.cpp
  CalculateTriangleAreasFilterTest.cpp
  ChangeAngleRepresentationTest.cpp
  CombineAttributeArraysTest.cpp
  ComputeMomentInvariants2DTest.cpp
  ComputeFeatureRectTest.cpp
  ConditionalSetValueTest.cpp
  ConvertColorToGrayScaleTest.cpp
  ConvertDataTest.cpp
  CopyDataObjectTest.cpp
  CopyFeatureArrayToElementArrayTest.cpp
  CoreFilterTest.cpp
  CreateAttributeMatrixTest.cpp
  CreateDataArrayTest.cpp
  CreateFeatureArrayFromElementArrayTest.cpp
  CreateGeometryTest.cpp
  CreateImageGeometryTest.cpp
  CropImageGeometryTest.cpp
  CropVertexGeometryTest.cpp
  DeleteDataTest.cpp
  DREAM3DFileTest.cpp
  ErodeDilateBadDataTest.cpp
  ErodeDilateCoordinationNumberTest.cpp
  ErodeDilateMaskTest.cpp
  ExecuteProcessTest.cpp
  ExtractComponentAsArrayTest.cpp
  ExtractInternalSurfacesFromTriangleGeometryTest.cpp
  ExtractVertexGeometryTest.cpp
  FeatureDataCSVWriterTest.cpp
  FillBadDataTest.cpp
  FindArrayStatisticsTest.cpp
  FindBiasedFeaturesTest.cpp
  FindBoundaryCellsTest.cpp
  FindDifferencesMapTest.cpp
  FindEuclideanDistMapTest.cpp
  FindFeatureCentroidsFilterTest.cpp
  FindFeatureClusteringTest.cpp
  FindFeaturePhasesFilterTest.cpp
  FindLargestCrossSectionsTest.cpp
  FindNeighborhoodsTest.cpp
  FindNeighborListStatisticsTest.cpp
  FindNeighborsTest.cpp
  FindNumFeaturesTest.cpp
  FindSurfaceAreaToVolumeTest.cpp
  FindSurfaceFeaturesTest.cpp
  FindTriangleGeomCentroidsTest.cpp
  FindTriangleGeomSizesTest.cpp
  FindVertexToTriangleDistancesTest.cpp
  FindVolFractionsTest.cpp
  GenerateColorTableTest.cpp
  IdentifySampleTest.cpp
  ImageContouringTest.cpp
  ImageGeomTest.cpp
  ImportCSVDataTest.cpp
  ImportBinaryCTNorthstarTest.cpp
  ImportHDF5DatasetTest.cpp
  ImportVolumeGraphicsFileTest.cpp
  InitializeDataTest.cpp
  InterpolatePointCloudToRegularGridTest.cpp
  IterativeClosestPointTest.cpp
  LaplacianSmoothingFilterTest.cpp
  MapPointCloudToRegularGridTest.cpp
  MinNeighborsTest.cpp
  MoveDataTest.cpp
  NearestPointFuseRegularGridsTest.cpp
  PartitionGeometryTest.cpp
  PipelineTest.cpp
  PointSampleTriangleGeometryFilterTest.cpp
  QuickSurfaceMeshFilterTest.cpp
  RawBinaryReaderTest.cpp
  RemoveFlaggedFeaturesTest.cpp
  RemoveFlaggedVerticesTest.cpp
  RemoveMinimumSizeFeaturesTest.cpp
  RenameDataObjectTest.cpp
  ReplaceElementAttributesWithNeighborValuesTest.cpp
  ResampleImageGeomTest.cpp
  RobustAutomaticThresholdTest.cpp
  RotateSampleRefFrameTest.cpp
  ScalarSegmentFeaturesFilterTest.cpp
  SetImageGeomOriginScalingFilterTest.cpp
  SharedFeatureFaceTest.cpp
  SplitAttributeArrayTest.cpp
  StlFileReaderTest.cpp
  TriangleCentroidTest.cpp
  TriangleDihedralAngleFilterTest.cpp
  TriangleNormalFilterTest.cpp
  WriteASCIIDataTest.cpp
  WriteBinaryDataTest.cpp
  WriteStlFileTest.cpp
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
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_test_data_1.tar.gz SHA512 6e21118a882c6a0cc54341eec8928b89ee84ac3a41b1d5b534193f4fabcb49c363db22028055622ad777787be0163cf5525e6c548c11c2c369748feb23031651)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_feature_centroid.tar.gz SHA512 e79a4c8e59bc856d40e91daf4cdce8b82c2e5ccfa6de51e23a0b8c6628282b03701bd5b5d7ddde76c4378142e3d9fe7c1cd6db261360c91751cf7c63747b054b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_feature_centroids.tar.gz SHA512 96ca08eca2ac3839a1d7ded6287c138cac49140d1b80713d747f6143a90b75335b351cd291e2e438621cdfb32bcb5f0ec5bf2f59ec0e0d61250e423f289bdad8)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_apply_transformation_to_geometry.tar.gz SHA512 d3510e4744de49cf98db295b6b338bd5b3199b817f7153212622292b772b7fee4f914017243845b2181d70194d618ac0582d209f96b1a0127e75b976d04f1634)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_erode_dilate_test.tar.gz SHA512 5f0773e5d296936effbb2239965f5847e7c18533b0a2c3ec6a1d6a83b03417e5b459cce29808c8e0273613b3b6fa032c675e84926eb35d8da8a6ddc0641a0ef5)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_fill_bad_data.tar.gz SHA512 6898d38754e80def4b8459227a4a9f99b67baaf6a72210c50ef8e98e1cc131b10da2f8793e807d9883c6949de6aa72f71fe65df89ddc0b39cca1d550191b6c45)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_biased_features.tar.gz SHA512 5a9df4c5a660768b19973b4ee3c6a59e1a997ea63823ef5931327a4857cc179d4f5dbf346130543d7ec13bee020c2caca483f1d4a730ee10a92b247cf3b0fe86)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_boundary_cells.tar.gz SHA512 bced17620701ca6f68e8477783b169fde07f500615ee3becbd06d6f8543b832b9e01252324b6a7dfa046c0deb4355eb5647586011d2c8bbed41332b3bdfe0c7e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_centroids.tar.gz SHA512 1eecd4eda617b6b13f3321b33d9cc35fd53f01cf19384c8922318639e86b765d381bd0a07d4dae14628c99cc0e1caffcf23aa667683732d3925307409bda9c28)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_clustering.tar.gz SHA512 37d8b5fccfc137662362dc4d931f468ff45ece9622390f39b90b54e73a1a14fb26d8d31b8f2f6d4fa22a57399a5bf36bb5dea03743c324f0617ba2cbc3683a39)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_shapes.tar.gz SHA512 657bb5015e731afad605274f01f38fa375b423a91159946c691361e69580d282909072fb026a778b09c0f2b6e2359549a4f138072b343a35a2c97170c1af1292)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_map_point_cloud_to_regular_grid.tar.gz SHA512 1592511d4fc810f31fda432bfdc061fd8b48fd2dc06a6961b695837de91faeed282ceb56f5635a28c8584e330309a1d112537fa844f3592a3ec2a109e6ccc356)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_input.tar.gz SHA512 f59d0a5c3b414ed11419a1cf1594f430d76a9b462086592a3c6bbf41b4c62b33d571981b9fc277e4c6ce43e0a870c02ec9e90d91b49720c8005b87ccbdb11a14)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_output.tar.gz SHA512 2ad1bc5e99eddf9939b52e16fcd1d8ca70ec37bfe9396b7123af3d8f11f83d015cf0bae2b5e903e65cec8e5d7bda328649042253a046d1e6c89b637e0a5faf91)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_replace_element_attributes_with_neighbor.tar.gz SHA512 319ebdf08b83ce5ec915afda8ee2af1e0952a3ce26b3e65a4171eb3125c7bc6613c3994610bf526f70413c00da9061c3e6c2d867643220d62faa8c3cd79a96cd)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_stats_test.tar.gz SHA512 cb1d1c004ab5a3cb29cc10f7b6c291dd5819e57e303242b8162cd0b268dea24b1d3e5e3811ec4f5ee216179b6eb6b81c76ee7d37b7c49e83c6ab336147b4b14e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_vertex_to_triangle_distances.tar.gz SHA512 d679cde985c326a249c2b52c6e5732dcead263127a8e7f5578c4bf450730a691c840af75a0162d2bbb38eb97f9572cf72a3e1b01d3fd6c670db59619f46aeec9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_volume_fraction_feature_count.dream3d.tar.gz SHA512 e4205186dee5b97bef3fb5f265bc12f8379a68860f70de33a483680eba0433a35c9c98db56bc92c949fb215f73f5f7698f83394697e81604c8c4b8a06874943f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME abaqus_hexahedron_writer_test.tar.gz SHA512 ae8626abfacdefeebed5c10c80808e1f047b8e4ba120bdcc0f6cde82508f4af24e6bd37d127be1b19b05df0d34efbfcd481426542cbb7f2ea599e7bfb32f5513)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME add_bad_data_test.tar.gz SHA512 cfb04a56e4d4c4098d30636922f35e5dc0a292b7e9a475fca745a7eeb820dcdde719fe62e371197bbca461047a4631e63d6f21cfd814b8755e09176951d5538c)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ascii_data_exemplars.tar.gz SHA512 86e2d61db98e301cf9b529ce2a86372704c9f723e3728fa3971a37aa31cff97dd6649c5095f0ab1508e9b74129342912b1fd55f9adc3803e57f3edfbb2510718)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME csv_test_file.tar.gz SHA512 558121e0893c6af2be6f803b6a4a04c9e8297e9865a57acc3fb1fda6fb5e5c2cf44b88adc998b43aa6e54faa10e09fbb69393f5c1036c32df926337963b31cc5)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Example.tar.gz SHA512 b7ea2b8bb5ecb29cb20d69e5820ebf513433c7740ce5cc9e6c1e44b539941dd3cf1a627442ace2cd430fe64437a479a62a172e6d398edf992ccdfc906ebceeba)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME export_files_test.tar.gz SHA512 c653cf435c843f676d495cf25a0329644bda7aee55ab1e5268efabd6afb6aafddb481ba12f69a3beb4882fd7b5d6a05095a748046b31add168e4c77eefa18388)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME FindSurfaceFeaturesTest.tar.gz SHA512 4a03393a60fedb1d932faef82bfca5d9da79e7adbfa61620b25ebac1a24ae554861700470d91b6f7958e8d07157ba9bedb1bdb625f0e84dfcb32df5ddb6e9911)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME flying_edges_exemplar.tar.gz SHA512 c83117f79b381814b349600e5c0c33874c5665e5e3ae09ddb60d0b619ea6459ae5a70c003f21d7229efbd1ce066246e379f8ff2fbc00290021a90b0cbfeea361)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME GenerateColorTableTest.tar.gz SHA512 2b660e327f730f868dd5ccf0bc39509d731b53011c755eda64f21cc2f28ff5a5c0c1b933666e2bca49b46cfb05d03ad3b9380d338bc2794cd562aef4e7480274)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME PartitionGeometryTest.tar.gz SHA512 8074dbcff86ed206d9185065a56258b766eb24367b3481e8600e9b48542de23dbd715ef8f9eca1c9718ca5e05a9c48997f613bde6f4357b8bba206241caecbbd)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ResampleImageGeom_Exemplar.tar.gz SHA512 464029b7354b96a943d75c495ef02bac0f834032e5a86576dde9afee51febff3fd6ffd7d4f8f1e9f8315d8cda36971df26601c7212e1876151109ca5428b8659)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Rotate_Sample_Ref_Frame_Test.tar.gz SHA512 75c02a11fbb06e9df464df542be8cecc82ec73a7324abf7f12a055570ecc32ec3add6c662ab071868f6708005a6027b89065dbd70605b576520d31e16ef3c372)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_h5ebsd.tar.gz SHA512 31e606285ea9e8235dcb5f608fd2b252a5ab1492abd975e5ec33a21d083aa9720fe16fb8f752742c140f40e963d692f1a46256b9d36e96b1b09796c1e4ea3db9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME volume_graphics_test.tar.gz SHA512 94d996fbf2b8b42cd715fb3adf33548f097970211e7a26eb9ccc5b073a78014eecbd10d40fc0451ff9cc7c92e23ed21582fc082d64e1f62b3714e4d9640c975f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME write_stl_test.tar.gz SHA512 560fe3f7d07bbda26c1d8097e1f3f7a7cf7287416eeac1f7c5ad3e32cb1b09ce05ee3a2741dde60e7a3d148d2685172e565f81d571e2ad0fc1adbd2753cfb3de)

endif()


# -----------------------------------------------------------------------------
# Include any example pipelines into the unit testing
# -----------------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Utility.cmake)

set(PREBUILT_PIPELINE_NAMES
  # Example or other test pipelines
  
  # Currently Broken; Waiting on another PR. https://github.com/BlueQuartzSoftware/complex/pull/472
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ApplyTransformation_Demo.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ApplyTransformation_Image.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ApplyTransformation_Node.d3dpipeline"

  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ArrayCalculatorExample.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ExecuteProcess.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Import_CSV_Data.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Import_STL_Model.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Triangle_Face_Data_Demo.d3dpipeline"

  # These are workflow files
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Import_ASCII_Data.d3dworkflow"

  # These depend on files that are output from OrientationAnalysis Plugin
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/FindBiasedFeatures.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/FindBoundaryCells.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Import_ASCII.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ReplaceElementAttributesWithNeighbor.d3dpipeline"

)
create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})

