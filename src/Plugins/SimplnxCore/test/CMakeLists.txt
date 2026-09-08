# ------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  # ReadDeformKeyFileV12Test.cpp
  AddBadDataTest.cpp
  AlignGeometriesTest.cpp
  AlignSectionsFeatureCentroidTest.cpp
  AlignSectionsListTest.cpp
  AppendImageGeometryTest.cpp
  ApplyTransformationToGeometryTest.cpp
  ApproximatePointCloudHullTest.cpp
  ArrayCalculatorTest.cpp
  ChangeAngleRepresentationTest.cpp
  CombineAttributeArraysTest.cpp
  CombineNodeBasedGeometriesTest.cpp
  CombineStlFilesTest.cpp
  CombineTransformationMatricesTest.cpp
  ComputeArrayHistogramByFeatureTest.cpp
  ComputeArrayHistogramTest.cpp
  ComputeArrayStatisticsTest.cpp
  ComputeBiasedFeaturesTest.cpp
  ComputeBoundaryCellsTest.cpp
  ComputeBoundaryElementFractionsTest.cpp
  ComputeBoundingBoxStatsTest.cpp
  ComputeCoordinatesImageGeomTest.cpp
  ComputeCoordinateThresholdTest.cpp
  ComputeDifferencesMapTest.cpp
  ComputeEuclideanDistMapTest.cpp
  ComputeFeatureBoundsTest.cpp
  ComputeFeatureCentroidsTest.cpp
  ComputeFeatureClusteringTest.cpp
  ComputeFeatureNeighborsTest.cpp
  ComputeFeaturePhasesBinaryTest.cpp
  ComputeFeaturePhasesFilterTest.cpp
  ComputeFeatureRectTest.cpp
  ComputeFeatureSizesTest.cpp
  ComputeGroupingDensityTest.cpp
  ComputeKMeansTest.cpp
  ComputeKMedoidsTest.cpp
  ComputeLargestCrossSectionsTest.cpp
  ComputeMomentInvariants2DTest.cpp
  ComputeNeighborhoodsTest.cpp
  ComputeNeighborListStatisticsTest.cpp
  ComputeNumFeaturesTest.cpp
  ComputeSurfaceAreaToVolumeTest.cpp
  ComputeSurfaceFeaturesTest.cpp
  ComputeTriangleAreasFilterTest.cpp
  ComputeTriangleGeomCentroidsTest.cpp
  ComputeTriangleGeomVolumesTest.cpp
  ComputeVectorColorsTest.cpp
  ComputeVertexToTriangleDistancesTest.cpp
  ComputeVolumeFractionsTest.cpp
  ConcatenateDataArraysTest.cpp
  ConditionalSetValueTest.cpp
  ConvertColorToGrayScaleTest.cpp
  ConvertDataTest.cpp
  CopyDataObjectTest.cpp
  CopyFeatureArrayToElementArrayTest.cpp
  CoreFilterTest.cpp
  CreateAMScanPathsTest.cpp
  CreateAttributeMatrixTest.cpp
  CreateColorMapTest.cpp
  CreateDataArrayAdvancedTest.cpp
  CreateDataArrayTest.cpp
  CreateFeatureArrayFromElementArrayTest.cpp
  CreateGeometryTest.cpp
  CreateImageGeometryTest.cpp
  CreatePythonSkeletonTest.cpp
  CropEdgeGeometryTest.cpp
  CropImageGeometryTest.cpp
  CropVertexGeometryTest.cpp
  DBSCANTest.cpp
  DeleteDataTest.cpp
  DREAM3DFileTest.cpp
  ErodeDilateBadDataTest.cpp
  ErodeDilateCoordinationNumberTest.cpp
  ErodeDilateMaskTest.cpp
  ExecuteProcessTest.cpp
  ExtractComponentAsArrayTest.cpp
  ExtractFeatureBoundaries2DTest.cpp
  ExtractInternalSurfacesFromTriangleGeometryTest.cpp
  ExtractPipelineToFileTest.cpp
  ExtractVertexGeometryTest.cpp
  FeatureFaceCurvatureTest.cpp
  FillBadDataTest.cpp
  FlyingEdges3DTest.cpp
  HierarchicalSmoothTest.cpp
  IdentifyDuplicateVerticesTest.cpp
  IdentifySampleTest.cpp
  ImageGeomTest.cpp
  InitializeDataTest.cpp
  InitializeImageGeomCellDataTest.cpp
  InterpolatePointCloudToRegularGridTest.cpp
  IterativeClosestPointTest.cpp
  KeepRemoveRankedFeaturesTest.cpp
  LabelTriangleGeometryTest.cpp
  LaplacianSmoothingFilterTest.cpp
  ExtractTripleLinesTest.cpp
  M3CSurfaceMeshingTest.cpp
  MapPointCloudToRegularGridTest.cpp
  MoveDataTest.cpp
  MultiThresholdObjectsTest.cpp
  NearestPointFuseRegularGridsTest.cpp
  OmitBoundingBoxSkinTest.cpp
  PadImageGeometryTest.cpp
  PartitionGeometryTest.cpp
  PipelineTest.cpp
  PointSampleEdgeGeometryTest.cpp
  PointSampleTriangleGeometryFilterTest.cpp
  QuickSurfaceMeshFilterTest.cpp
  RandomizeFeatureIdsTest.cpp
  ReadBinaryCTNorthstarTest.cpp
  ReadCSVFileTest.cpp
  ReadHDF5DatasetTest.cpp
  ReadImageStackTest.cpp
  ReadImageTest.cpp
  ReadNIfTIFileTest.cpp
  ReadRawBinaryTest.cpp
  ReadStlFileTest.cpp
  ReadStringDataArrayTest.cpp
  ReadTextDataArrayTest.cpp
  ReadVolumeGraphicsFileTest.cpp
  ReadVtkStructuredPointsTest.cpp
  ReadZeissTxmFileTest.cpp
  RegularGridSampleSurfaceMeshTest.cpp
  RegularizeZSpacingTest.cpp
  RemoveFlaggedEdgesTest.cpp
  RemoveFlaggedFeaturesTest.cpp
  RemoveFlaggedTrianglesTest.cpp
  RemoveFlaggedVerticesTest.cpp
  RenameDataObjectTest.cpp
  ReplaceElementAttributesWithNeighborValuesTest.cpp
  RequireMinimumSizeFeaturesTest.cpp
  RequireMinNumNeighborsTest.cpp
  ResampleImageGeomTest.cpp
  ResampleRectGridToImageGeomTest.cpp
  ReshapeDataArrayTest.cpp
  ReverseTriangleWindingTest.cpp
  RobustAutomaticThresholdTest.cpp
  RotateSampleRefFrameTest.cpp
  ScalarSegmentFeaturesTest.cpp
  SetImageGeomOriginScalingFilterTest.cpp
  SharedFeatureFaceTest.cpp
  SilhouetteTest.cpp
  SliceTriangleGeometryTest.cpp
  SplitDataArrayByComponentTest.cpp
  SplitDataArrayByTupleTest.cpp
  SurfaceMeshingTestUtilsTest.cpp
  SurfaceNetsFaceLabelsTest.cpp
  SurfaceNetsNodeTypesTest.cpp
  SurfaceNetsTest.cpp
  TriangleCentroidTest.cpp
  TriangleDihedralAngleFilterTest.cpp
  TriangleNormalFilterTest.cpp
  UncertainRegularGridSampleSurfaceMeshTest.cpp
  VerifyTriangleWindingTest.cpp
  WriteAbaqusHexahedronTest.cpp
  WriteASCIIDataTest.cpp
  WriteAvizoRectilinearCoordinateTest.cpp
  WriteAvizoUniformCoordinateTest.cpp
  WriteBinaryDataTest.cpp
  WriteFeatureDataCSVTest.cpp
  WriteImageTest.cpp
  WriteLAMMPSFileTest.cpp
  WriteLosAlamosFFTTest.cpp
  WriteNodesAndElementsFilesTest.cpp
  WriteSPParksSitesTest.cpp
  WriteSPParksSitesTest.cpp
  WriteStlFileTest.cpp
  WriteVtkRectilinearGridTest.cpp
)

create_simplnx_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
  FILTER_LIST ${${PLUGIN_NAME}UnitTest_SRCS})

# ------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC [name of library])

# The ReadNIfTIFileTest synthesizes .nii and .nii.gz files via zlib
find_package(ZLIB REQUIRED)
target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC ZLIB::ZLIB)

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
  ${simplnx_SOURCE_DIR}/test
)

# -----------------------------------------------------------------------------
# Here we are going to setup to download and decompress the test files. In order
# to add your own test files you will need to tar.gz the test file, compute the
# SHA 512 Hash of the file and then upload the file to
# https://github.com/bluequartzsoftware/simplnx/releases/tag/Data_Archive.
#
# Go to the web site above, "edit" the release, add the filename and SHA 512 to
# the table and then upload your compressed file.
# Save the release so that the repo is updated
# -----------------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}" AND SIMPLNX_DOWNLOAD_TEST_FILES)
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_fill_bad_data.tar.gz SHA512 73b6835252135619325d9b02b48286ca95f0235820610bddcc43ec0a3f075337481bace00bf20e9d59744f1d4024cc18be624ec522922a6461b55ae8dbb0e628)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_Goldfeather.tar.gz SHA512 3ad4a8f2bd578d81ce1ca67213ec846b7de4394e0c0e7796077bac51f7b58ae36340e2abd315665d9ee5fe745315458d5c1559766765e20dbe2c35eb693e8e26 INSTALL COPY_DATA)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_spparks_sites_writer.tar.gz SHA512 c2429fbe9efea07eada95c670266542a18c74670dff7c80f657a17fa84684c4852180dc36527f9138b27eeea5cba2f1dfe21e0dd15e5837596865484ebf3450a)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_test_data_1_v2.tar.gz SHA512 585b51ba1da9784a204fe88073ca562b45afd7007cf451b0193079b885c4b4caff7cf21b13e016433b84155546ac0f73f003a8b8ebb1c58360b2c56de3027d6c)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_feature_centroids.tar.gz SHA512 96ca08eca2ac3839a1d7ded6287c138cac49140d1b80713d747f6143a90b75335b351cd291e2e438621cdfb32bcb5f0ec5bf2f59ec0e0d61250e423f289bdad8)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_avizo_writers.tar.gz SHA512 f776eb5c86e06bde0fb1ee76dbdf95e4fd1457697033b2c639cac376db3ba0b05410ed4074fb10a47dd26ef79c78a02b5bb77c04cfe1299e8a33d8b3bff09749)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_combine_stl_files_v2.tar.gz SHA512 40235976b9897365c338587056dc9ba90466b3eca58d6b310d59f7df16592b53647349daa1c7247cdcf01b5940596ad53d39eb734549dda4e829ae93a5928a4a)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_erode_dilate_test.tar.gz SHA512 5f0773e5d296936effbb2239965f5847e7c18533b0a2c3ec6a1d6a83b03417e5b459cce29808c8e0273613b3b6fa032c675e84926eb35d8da8a6ddc0641a0ef5)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_fill_bad_data.tar.gz SHA512 6898d38754e80def4b8459227a4a9f99b67baaf6a72210c50ef8e98e1cc131b10da2f8793e807d9883c6949de6aa72f71fe65df89ddc0b39cca1d550191b6c45)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_biased_features.tar.gz SHA512 5a9df4c5a660768b19973b4ee3c6a59e1a997ea63823ef5931327a4857cc179d4f5dbf346130543d7ec13bee020c2caca483f1d4a730ee10a92b247cf3b0fe86)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_boundary_cells.tar.gz SHA512 bced17620701ca6f68e8477783b169fde07f500615ee3becbd06d6f8543b832b9e01252324b6a7dfa046c0deb4355eb5647586011d2c8bbed41332b3bdfe0c7e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_boundary_element_fractions.tar.gz SHA512 b267b902c87030f7508882e77d3f4483a9d4e351da78061b1b845b0301529a3532ac60934748eec1b4f8e900778af78244133b24607509b80c5cd49bc5f07a49)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_centroids.tar.gz SHA512 1eecd4eda617b6b13f3321b33d9cc35fd53f01cf19384c8922318639e86b765d381bd0a07d4dae14628c99cc0e1caffcf23aa667683732d3925307409bda9c28)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_clustering.tar.gz SHA512 37d8b5fccfc137662362dc4d931f468ff45ece9622390f39b90b54e73a1a14fb26d8d31b8f2f6d4fa22a57399a5bf36bb5dea03743c324f0617ba2cbc3683a39)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_interpolate_point_cloud_to_regular_grid.tar.gz SHA512 287500082913dfbf62c5f66ec5185c3db53cedc20a2c89c28b23a208ddfca47c377d557d2f509c1d72f3aff8aadc3058e6932159a6cef73f33227ad433f14752)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_map_point_cloud_to_regular_grid.tar.gz SHA512 1592511d4fc810f31fda432bfdc061fd8b48fd2dc06a6961b695837de91faeed282ceb56f5635a28c8584e330309a1d112537fa844f3592a3ec2a109e6ccc356)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_input.tar.gz SHA512 f59d0a5c3b414ed11419a1cf1594f430d76a9b462086592a3c6bbf41b4c62b33d571981b9fc277e4c6ce43e0a870c02ec9e90d91b49720c8005b87ccbdb11a14)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_output.tar.gz SHA512 2ad1bc5e99eddf9939b52e16fcd1d8ca70ec37bfe9396b7123af3d8f11f83d015cf0bae2b5e903e65cec8e5d7bda328649042253a046d1e6c89b637e0a5faf91)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_replace_element_attributes_with_neighbor.tar.gz SHA512 319ebdf08b83ce5ec915afda8ee2af1e0952a3ce26b3e65a4171eb3125c7bc6613c3994610bf526f70413c00da9061c3e6c2d867643220d62faa8c3cd79a96cd)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_resample_rect_grid_to_image_geom.tar.gz SHA512 58cabfb83deeef7aff1252963a8cb378745ca00fc120a9623f699b4c9c618d216c4d3fa88e964772706a0096e979abc741be7ba6f96a2398162b3f6d4ec8ec13)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_sample_surface_mesh.tar.gz SHA512 b1eec1127d125303a4757a482a7fd4b6c0e42241f01f8afabec9a15dc1ffa6d732afc11b4623aaf81c7a9e53de59423ef4bbc470710c31fe9a5c61916bdf544e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_stats_test_v2.tar.gz SHA512 e84999dec914d81efce4fc4237c49c9bf32e48381b1e79f58aa4df934f0d7606cd7a948f9a5e7b17a126a7944cc531b531cfdc70756ca3e2207b20734e089723)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_vertex_to_triangle_distances.tar.gz SHA512 d679cde985c326a249c2b52c6e5732dcead263127a8e7f5578c4bf450730a691c840af75a0162d2bbb38eb97f9572cf72a3e1b01d3fd6c670db59619f46aeec9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_volume_fraction_feature_count.dream3d.tar.gz SHA512 e4205186dee5b97bef3fb5f265bc12f8379a68860f70de33a483680eba0433a35c9c98db56bc92c949fb215f73f5f7698f83394697e81604c8c4b8a06874943f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_write_stl_file_test.tar.gz SHA512 05455dfb57724a14a3b2d92ad02ac335b5596ca4f353830ee60d84ffc858523136140328a1c95b6693331be9dc5c40a26e76f45ee54ab11e218f5efb6b2a4c38)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 7_0_abaqus_hexahedron_writer_test.tar.gz SHA512 e28ec04784186a3d1d13d65d9df57ceb5dfc8d673b1815ce487244bbac6dea52ddfe27c980650de1ebc0eabad6cc6a40f9cc8ce025fca4d1d471b6920ade7825)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 7_0_SurfaceMesh_Test_Files_v2.tar.gz SHA512 6a2a8488385e8869a8ce49647a083013a45d0c99d83f9b01a5ac23d4d26dad0a3c43f8c8db47b4579b274df3cd2e8f8ce81c045e6ea537bdbcddad7432cef37b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 7_2_PadImageGeometry.tar.gz SHA512 75b168ef53d7336f024648199856fe3c2b4a74df40b0a6c0cb4e7c6eb1181c3476dde12de8a0c7c4d40b4c5c7d0d4ba6b5ffd2c9204a8da2ac3688d92aa6650d)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 7_ReadStringArray.tar.gz SHA512 594b9a4605db8de0f6682884b07385938284e5309a5c56514dce3206ede5cdbdb1cf8629eb5bd5b77cbe59ee650ee381988cfc9ac7aa12862ec944926db4b530)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME add_bad_data_test.tar.gz SHA512 cfb04a56e4d4c4098d30636922f35e5dc0a292b7e9a475fca745a7eeb820dcdde719fe62e371197bbca461047a4631e63d6f21cfd814b8755e09176951d5538c)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME align_sections_feature_centroids.tar.gz SHA512 06a4f576108e96fff49241ed39bdcaa90b96ba9b6fa91f420bde9d797a21fc58880b52f4d070c0efbf5b193e7c9f0c0383c140db404c5e74c3ff8ce3b4c3ca9b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME apply_transformation_to_geometry.tar.gz SHA512 1613078988b145615003d18f4fda4405af914487503f243b9e0ee55951be60041ab8764b522ca8268d6af0fd9bc6892bb8f99e17c1472403d477221715698dbc)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ascii_exemplars.tar.gz SHA512 e497e2b2d9ab9260f58b1ebf26cacee82323541e166cef2050d08526c27cf056e55ca19e150e6b6c5b5ee5b3ad66452dedcd1f1e6ae15aa5050a2e410078593f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME bin_feature_phases.tar.gz SHA512 e43805c5a69d08ce6d657f8fa3fb9fec7d174f2329412fa62ba95ac6d82f1983ed2c5a641d81651ba86e6eeb5d20602b6e131f34094160305c4bf03cfbd06be1)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME combine_node_based_geometries.tar.gz SHA512 beb2186d3522f6ab20d57bb5a0809fea95a5a3ddb2f9a70e1fc049d1ee05b2aa6f3165fa82cc5b6b3ba2e41457d8b4b86c3987c6ee6f05efdb23760b922b4d18)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME combine_transformation_matrices_test.tar.gz SHA512 f4f44e39e2f42114e910f1e235e50bb8cad76b03dee02020784e34b89f58977ea5c9fe47c045118ef3d26dd08e12653f81bf5db62c172932c668c078d0a1a875)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME dbscan_test.tar.gz SHA512 77d7886e2550b63176b564e827d7de320b5a28b1c8a55bf107d53acd6962757275bc86b3382ff789f612a6838f55ab8f4af29435aec83a7a804b9487e57a6386)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME export_files_test.tar.gz SHA512 e7230c169a94bc429315e89bc5d96ef656fab209ac4225a010c15157204c428930fb1b4c8a39a694faa4574df5aa14ab70e0b755a5036c997fbcd2fb2d6009c2)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME extract_internal_surface.tar.gz SHA512 86d47a9850d03486a62707f2f0c48fe7bc5d88a0b71fc286f74ad13a1f3c982f36f55ac65dda43b67826b14b4fce0dc2d29dd5adbee6338a887a12f42e879ed7)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME extract_vertex_geometry.tar.gz SHA512 b34c877e6a150eb461aa21afc33529a8cde07e32d4c1e788cb2c98751ebf1d8f82ca9da010c8c3de6f15e53e9a8d030d62c93173815b101fe676edccac59ad0a)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ExtractFeatureBoundaries2D.tar.gz SHA512 3e4ce48bc886cb8e5ecb9e8e7ceea23cb0aa18e5a2c3b47da337a9ea59ddc3eb110d941ef35a6e408482a9ad9292fcc2a52b072cc967e8542fa9f62d3311849b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME FindSurfaceFeaturesTest.tar.gz SHA512 4a03393a60fedb1d932faef82bfca5d9da79e7adbfa61620b25ebac1a24ae554861700470d91b6f7958e8d07157ba9bedb1bdb625f0e84dfcb32df5ddb6e9911)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME flying_edges_exemplar_v2.tar.gz SHA512 93cab3473ca16464a00866978477e3f56e8cbea15fc0d9b989c945a352ee8f67b5c253907b24fc23b20652976d6e2cb8872a302c37b4c89c1b21fca13c080f0f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME generate_color_table_test.tar.gz SHA512 b5683c758964eb723267400b14047f8adb0d5365ee9ca93d1a6940e9b6ad198cd4739c1ca799eb787b7706e668dbc16ab8243642034cdba5b71d64c27e682d3f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME generate_vector_colors.tar.gz SHA512 a30ac9ac35002f4a79b60a02a86de1661c4828c77b45d3848c6e2c37943c8874756f0e163cda99f2c0de372ff1cd9257288d5357fa3db8a539428df713325ee7)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME identify_sample.tar.gz SHA512 0c1da22d411ac739d3e90618141a6eab71705b47de6d4cc7501e9408bf6fcbadd46738f5e86a80ab2e0bc2344c23584cc2b89f2102c13073490e1817797ec9bc)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME image_coords_test.tar.gz SHA512 bca23117b00e77e0401b83098caba6c4586f3d7068eb5b513da1be365a3b56aabe4a27106cb1e30c5f06192c277fb7cce208e7b180f93807d72f5f2e01e26c43)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME initialize_data_test_files.tar.gz SHA512 f04fe76ef96add4b775111ae7fc95233a7748a25501d9f00a8b2a162c87782b8cd2813e6e46ba7892e721976693da06965e624335dbb28224c9c5b877a05aa49)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME k_files_v2.tar.gz SHA512 d5479ad573f8ce582e32c9f7a4824885b965f08cd8ed6025daf383758088716b1e9109905ab830b9f4d4f9df0145cbeb2534258237846c47e4a046b24fb3f72c)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME label_triangle_geometry_test_v2.tar.gz SHA512 3f2009a1e37d8f5c983ee125335f42900f364653a417d8c49c9409a41a032c6f77a80b0623f5c71b1ae736f528d3b07949f7703bc342876f5424d884c3d4226c)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME LosAlamosFFTExemplar.tar.gz SHA512 c4f2f6cbdaea4512ca2911cb0c3169e62449fcdb7cb99a5e64640601239a08cc09ea35f81f17b056fd38da85add95a37b244b7f844fe0abc944c1ba2f7514812)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME PartitionGeometryTest.tar.gz SHA512 8074dbcff86ed206d9185065a56258b766eb24367b3481e8600e9b48542de23dbd715ef8f9eca1c9718ca5e05a9c48997f613bde6f4357b8bba206241caecbbd)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME point_sample_edge_geometry.tar.gz SHA512 86ee348aac83da0b29c820851bf3cfb03d94d17c8249c8754b3471db3566467c814ce5868526fa35389d32832fbf2991d62c8a5a8de815594bac34176e17d3d6)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME QuickSurfaceMeshTest_v2.tar.gz SHA512 03c283cf3cc5d8c03ec75a18ffbf568f71ed3e690c8b746349dad3d59ee2d76fffcba9faa2fc859849c2ca7b1a8b11295c8fa4975246f6b9bc7eff1a15cb10c7)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME read_vtk_structured_points_test.tar.gz SHA512 e7a07a4e3901204c2562754cd71e0fdba1a46de2a5135bad2b6d66b40eefd0e63bed4dbe0ccd6ccadafb708ef63e20635d080aa3a35c172c4ced6986e0f75d5c)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ReadSTLFileTest.tar.gz SHA512 975587206625ffa183160308934e767347de55a34a16272cf5c121114efa286b3c6939e3c6a397e8728fdefe1771bc024bd4c9b409afdff0b76f2f56fcb9eb69)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ReadZeissTxmFileTest_v2.tar.gz SHA512 2d962655d919480fa7baf0c8720affbcbd1182e8482480f5413b53ead41787d35d0f15487219163ba0dcffad69be90e627730fc780107d80df3d97e14fb15cac)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME remove_flagged_elements_data.tar.gz SHA512 6bd5b1d93a100bcf8e6d9aa88243cadf51c9f0dee85fc3c054f5fe489b6720991b41390cf4682862483dfee11595a2e384d8698f68c0245cf0521d275db450ee)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ResampleImageGeom_Exemplar_2.tar.gz SHA512 0a5dc046410a9d93682a40581b169cf3ca5b516623d90eb081de8a8623c55de45a2b00427cd298d6f17fb84bb93dee061a96a4b4386392f925ecdc3fbfe6e5a6)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ResampleImageGeom_Exemplar.tar.gz SHA512 464029b7354b96a943d75c495ef02bac0f834032e5a86576dde9afee51febff3fd6ffd7d4f8f1e9f8315d8cda36971df26601c7212e1876151109ca5428b8659)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME reverse_triangle_winding.tar.gz SHA512 63247d7b8a0deee2fdb737527506312827331861758cd6106974d3aa1b9cb9c1d3b85d4b135e3eda27ac98e891198b4ee0498077ab231127dbb3dd8e83a5ea14)
  # Rotate_Sample_Ref_Frame_Test_v2/v3 archives retired 2026-07 during V&V: the RotateSampleRefFrame test now uses an
  # inlined, hand-derived Class 1 (analytical permutation) oracle instead of a golden-file exemplar.
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME sample_surface_mesh_regular_grid_v1.tar.gz SHA512 511cda431a5139bc81899b6ce22bc644f861df5b3cb939d058866b8fb1070542285f6bc889c736c6f39384b6443983cdec221ee15e70d063c95887285475cc17)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME segment_features_neighbor_scheme_test.tar.gz SHA512 4700b42eb3a9058dc08520c786d0e6aa20e45bfa06233952462c0c8a428e437406f530125822f1742dfe57405be66e4f0fd5ac2f76db4a13379a7ae189d370cd)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_dream3d_v3.tar.gz SHA512 bfa9547e787b0f8e8122702da0eb4e519f48a48bf4f94aad020f72479d071d32dfc96a1425705874c68507a61ed391d28606d9c4f4acd559043ef0ace64fd33f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_h5ebsd.tar.gz SHA512 31e606285ea9e8235dcb5f608fd2b252a5ab1492abd975e5ec33a21d083aa9720fe16fb8f752742c140f40e963d692f1a46256b9d36e96b1b09796c1e4ea3db9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME SurfaceMeshTest.tar.gz SHA512 a74e9fa40ccec78174dbbac90969bfa17367c3a7e14b7b84624032317c351de89957750803d7cb43c67dec19281ee4f647de022d474566fd43e25b8230cce6d6)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME SurfaceNetsTest_v4.tar.gz SHA512 9a89b55630120dc9c2701500e81da3f8d126fbc6e8a2de8ec21e1a8ef97ae90fda6c7a093a8217c65e1db5f5cebd091c37c8f6561cd44d3def911c8acbf83e49)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME volume_graphics_test.tar.gz SHA512 94d996fbf2b8b42cd715fb3adf33548f097970211e7a26eb9ccc5b073a78014eecbd10d40fc0451ff9cc7c92e23ed21582fc082d64e1f62b3714e4d9640c975f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME vtk_rectilinear_grid_writer.tar.gz SHA512 9fef02b5269609503d03dd0126cc635cc1c1156894cff18b0184b334d705b850ca1a06ae0d1c66a352a32dd9ad9fb74f24255c7de1399b06bbec7d2e2b41941b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME winding_quick_surface_mesh.tar.gz SHA512 ed9944b969c4c2feea8902f98d03024248780607848a40ad374d9a1c7861df1d971a355d779a7af8c775f2225b6364b9a17d61c2350375293e531bcee59255b7)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME winding_surface_nets.tar.gz SHA512 b45567fd89ea8ebac4764b37491041afa04516fee4b11b5c22d9d3a03c988e335f97395539438d008a7d0f006375a6ec0c62df2b3929ac59671f2e066bc2123f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME write_lammps_test.tar.gz SHA512 82bb5360b76e857f3233d37733c602f67fd2ac667e49b24741a70ab649e8046fb7905493df37d142808b740c2771fe7cdccd71c9d70679afafe398529ee5771e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME write_stl_overflow_test.tar.gz SHA512 d8f8eac901479100ffb5813b3ac72c86da496d3620d406f8691adc0d95eb4670bf4e887f57ccc702bac7e6eeaffdf61db9b4c06157423845dd198722870b0c0b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME compute_grouping_densities_v2.tar.gz SHA512 3aaabb63c4fa16f7fa192ae4ee9dbba9394ec7f1cd19aff55e399a624d495a3a778c7f6f282911f681e85cea99e4c6d15344274e9107f337af7d4a19f93784ff)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME identify_sample_v2.tar.gz SHA512 a7ffac3eaad479c07215c1dd16274c45a52466708a9d27b5f85a29b0eba3b6705b627e1052a7a27e9bfe89cd6e7df673beb7a1e98b262b6c52ea383b4848ac31)

endif()

# -----------------------------------------------------------------------------
# Include any example pipelines into the unit testing
# -----------------------------------------------------------------------------
include(${simplnx_SOURCE_DIR}/cmake/Utility.cmake)

set(PREBUILT_PIPELINE_NAMES

  # Example or other test pipelines
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ApplyTransformation_Demo.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ApplyTransformation_Image.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ApplyTransformation_Node.d3dpipeline"

  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ArrayCalculatorExample.d3dpipeline"

  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ExecuteProcess.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Read_Data_Files/Import_CSV_File.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Read_Data_Files/Import_STL_Model.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Triangle_Face_Data_Demo.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Remove_Flagged_Triangles.d3dpipeline"

  # These are workflow files
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Import_ASCII_Data.d3dworkflow"

  # These depend on files that are output from OrientationAnalysis Plugin
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ComputeBiasedFeatures.d3dpipeline"
  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ComputeBoundaryCells.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Read_Data_Files/Import_Text_Data.d3dpipeline"

  # "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ReplaceElementAttributesWithNeighbor.d3dpipeline"
)
create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})
