# ------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

# ------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  AlignSectionsMisorientationTest.cpp
  AlignSectionsMutualInformationTest.cpp
  BadDataNeighborOrientationCheckTest.cpp
  ConvertQuaternionTest.cpp
  ConvertOrientationsTest.cpp
  CreateEnsembleInfoTest.cpp
  EBSDSegmentFeaturesFilterTest.cpp
  EbsdToH5EbsdTest.cpp
  EnsembleInfoReaderTest.cpp
  ExportGBCDGMTFileTest.cpp
  ExportGBCDTriangleDataTest.cpp
  FindAvgOrientationsTest.cpp
  FindFeatureReferenceMisorientationsTest.cpp
  FindGBCDTest.cpp
  FindKernelAvgMisorientationsTest.cpp
  FindMisorientationsTest.cpp
  FindSchmidsTest.cpp
  FindShapesFilterTest.cpp
  FindTriangleGeomShapesTest.cpp
  GenerateFaceIPFColoringTest.cpp
  GenerateFaceMisorientationColoringTest.cpp
  GenerateFZQuaternionsTest.cpp
  GenerateGBCDPoleFigureTest.cpp
  GenerateIPFColorsTest.cpp
  GenerateQuaternionConjugateTest.cpp
  MergeTwinsTest.cpp
  NeighborOrientationCorrelationTest.cpp
  ReadAngDataTest.cpp
  ReadCtfDataTest.cpp
  ReadH5EbsdTest.cpp
  RotateEulerRefFrameTest.cpp
  RodriguesConvertorTest.cpp
)
set(DISABLED_TESTS
  # ConvertHexGridToSquareGridTest.cpp    # MISSING 1 or more Parameter Implementations
  CreateLambertSphereTest.cpp
  EMsoftSO3SamplerTest.cpp

  FindAvgCAxesTest.cpp
  FindBoundaryStrengthsTest.cpp
  FindCAxisLocationsTest.cpp
  FindDistsToCharactGBsTest.cpp
  FindFeatureNeighborCAxisMisalignmentsTest.cpp
  FindFeatureReferenceCAxisMisorientationsTest.cpp
  FindGBCDMetricBasedTest.cpp
  FindGBPDMetricBasedTest.cpp
  FindSlipTransmissionMetricsTest.cpp
  FindTwinBoundariesTest.cpp
  FindTwinBoundarySchmidFactorsTest.cpp
  GenerateOrientationMatrixTransposeTest.cpp
  INLWriterTest.cpp

  # ImportEbsdMontageTest.cpp    # MISSING 1 or more Parameter Implementations
  # ImportH5EspritDataTest.cpp    # MISSING 1 or more Parameter Implementations
  # ImportH5OimDataTest.cpp    # MISSING 1 or more Parameter Implementations
  # OrientationUtilityTest.cpp    # MISSING 1 or more Parameter Implementations

  ReplaceElementAttributesWithNeighborValuesTest.cpp
  Stereographic3DTest.cpp
  WritePoleFigureTest.cpp
  WriteStatsGenOdfAngleFileTest.cpp
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
target_sources(${PLUGIN_NAME}UnitTest
# PUBLIC
# additional header files
PRIVATE
${CMAKE_CURRENT_LIST_DIR}/${PLUGIN_NAME}TestUtils.hpp
)

# ------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}UnitTest
#   PUBLIC
#   ${CMAKE_CURRENT_LIST_DIR}/../../src
# )


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

option(COMPLEX_DOWNLOAD_TEST_FILES "Download the test files" ON)

if(EXISTS "${DREAM3D_DATA_DIR}" AND COMPLEX_DOWNLOAD_TEST_FILES) 
  if(NOT EXISTS ${DREAM3D_DATA_DIR}/TestFiles/)
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()

  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 12_IN625_GBCD.tar.gz SHA512 f696a8af181505947e6fecfdb1a11fda6c762bba5e85fea8d484b1af00bf18643e1d930d48f092ee238d1c19c9ce7c4fb5a8092d17774bda867961a1400e9cea)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_align_sections_mutual_information.tar.gz SHA512 363c52bf3b7677e06508fdb92350457d37bbd58ab2d26259e61ec9317f9146b85ec50c58151f69a0b38fc487f79af2896c8fef4e98d1e87c29af1cf19cbf884b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_5_h5ebsd_exemplar.tar.gz SHA512 9e5936b6e8e6fd51ecc7c17521ff29898bb0e8bc3d516741c47ae65577b4f2e0f3df8f485f19ecf93f1848b70aeeb98b91c2c5742965107acabeaa40182d898d)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_misorientation.tar.gz SHA512 2343654a8bcb344fcc613b6715c1e0b2c780fedbdf06cc8e5306b23f9d7908d5eef8faff7e3f0dd6f7ac734a1c6e2b376832bed38548288cd5e9e0af1b5602a8)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_ebsd_segment_features.tar.gz SHA512 acbb493a0668e0115ac49d4fedbbf7600759b9a66deb5d1004c2749a61d2bad2fcc60344bf72b2aeda5c8c098f458949dd1f8d58cb21682fa1393dfb7d0a1b84)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_Small_IN100_GBCD.tar.gz SHA512 543e3bdcee24ff9e5cd80dfdedc39ceef1529e7172cebd01d8e5518292ffdf4e0eb2e79d75854cb3eaca5c60e19c861ca67f369e21b81c306edb66327f47a1e3)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_stats_test.tar.gz SHA512 cb1d1c004ab5a3cb29cc10f7b6c291dd5819e57e303242b8162cd0b268dea24b1d3e5e3811ec4f5ee216179b6eb6b81c76ee7d37b7c49e83c6ab336147b4b14e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME bad_data_neighbor_orientation_check.tar.gz SHA512 6452cfb1f2394c10050082256f60a2068cfad78ef742e9e35b1d6e63b3fb7c35c9fe7bbe093bed4dbb4e758c49ec6da7b1f7e2473838a0421f39fbdd9f4a2f76)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME neighbor_orientation_correlation.tar.gz SHA512 122367452174ade2f24dde7a4610bddc4f147a223722d9b30c1df9eaa2cd2bf25e1c7957aba83f3f9de79b4eadd79339b848f9530d1ebf44c69244ea5442cf85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_Ang_Files.tar.gz SHA512 79e9f6948d4e8e06187e11216a67596fa786ffd2700e51f594ad014090383eb8bcc003e14de2e88082aa9ae512cc4fc9cee22c80066fc54f38c3ebc75267eb5b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_dream3d.tar.gz SHA512 6dd8a3412532bdc7481f7781c7087b4477c6a1efbe6b214f997dad30c53c59714a751be522f084b98065fe75100c74df901bb8af2f512ef47344d8f7941575cf)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME so3_cubic_high_ipf_001.tar.gz SHA512 dfe4598cd4406e8b83f244302dc4fe0d4367527835c5ddd6567fe8d8ab3484d5b10ba24a8bb31db269256ec0b5272daa4340eedb5a8b397755541b32dd616b85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_h5ebsd.tar.gz SHA512 31e606285ea9e8235dcb5f608fd2b252a5ab1492abd975e5ec33a21d083aa9720fe16fb8f752742c140f40e963d692f1a46256b9d36e96b1b09796c1e4ea3db9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME align_sections.tar.gz SHA512 b6892e437df86bd79bd2f1d8f48e44d05bfe38b3453058744320bfaf1b1dc461a484edc9e593f6b9de4ad4d04c41b5dbd0a30e6fc605341d046aec4c3062e33e)

endif()

# -----------------------------------------------------------------------------
# Include any example pipelines into the unit testing
# -----------------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Utility.cmake)

# These should be relative to the 
set(PREBUILT_PIPELINE_NAMES
  # THIS GROUP MUST GO FIRST because the .h5ebsd file is needed for some of the examples below
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Reconstruction/(01) Small IN100 Archive.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Reconstruction/(08) Small IN100 Full Reconstruction.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Statistics/(01) Small IN100 Morphological Statistics.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Statistics/(05) Small IN100 Crystallographic Statistics.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(01) Small IN100 Quick Mesh.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(02) Small IN100 Smooth Mesh.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(03) Small IN100 Mesh Statistics.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(04) Small IN100 GBCD.d3dpipeline"

  # Example or other test pipelines
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/AlignSectionsMutualInformation.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/CI_Histogram.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EnembleInfoReader.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ReadAng.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/ReadCTF.d3dpipeline"
)

# -----------------------------------------------------------------------------
# These pipelines need the ITKImageProcessing Plugin to write out Images
# -----------------------------------------------------------------------------
if(TARGET ITKImageProcessing)
  set(PREBUILT_PIPELINE_NAMES
    ${PREBUILT_PIPELINE_NAMES}
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/Edax_IPF_Colors.d3dpipeline"
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/TxCopper_Exposed.d3dpipeline"
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/TxCopper_Unexposed.d3dpipeline"
  )
endif()

create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})
