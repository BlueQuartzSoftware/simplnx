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
  CAxisSegmentFeaturesTest.cpp
  ConvertHexGridToSquareGridTest.cpp
  ConvertOrientationsTest.cpp
  ConvertQuaternionTest.cpp
  CreateEnsembleInfoTest.cpp
  EBSDSegmentFeaturesFilterTest.cpp
  EbsdToH5EbsdTest.cpp
  EnsembleInfoReaderTest.cpp
  ExportGBCDGMTFileTest.cpp
  ExportGBCDTriangleDataTest.cpp
  FindAvgCAxesTest.cpp
  FindAvgOrientationsTest.cpp
  FindBoundaryStrengthsTest.cpp
  FindCAxisLocationsTest.cpp
  FindFeatureNeighborCAxisMisalignmentsTest.cpp
  FindFeatureReferenceCAxisMisorientationsTest.cpp
  FindFeatureReferenceMisorientationsTest.cpp
  FindGBCDMetricBasedTest.cpp
  FindGBCDTest.cpp
  FindGBPDMetricBasedTest.cpp
  FindKernelAvgMisorientationsTest.cpp
  FindMisorientationsTest.cpp
  FindSchmidsTest.cpp
  FindShapesFilterTest.cpp
  FindSlipTransmissionMetricsTest.cpp
  FindTriangleGeomShapesTest.cpp
  GenerateFaceIPFColoringTest.cpp
  GenerateFaceMisorientationColoringTest.cpp
  GenerateFZQuaternionsTest.cpp
  GenerateGBCDPoleFigureTest.cpp
  GenerateIPFColorsTest.cpp
  GenerateQuaternionConjugateTest.cpp
  ImportH5EspritDataTest.cpp
  ImportH5OimDataTest.cpp
  INLWriterTest.cpp
  MergeTwinsTest.cpp
  NeighborOrientationCorrelationTest.cpp
  ReadAngDataTest.cpp
  ReadCtfDataTest.cpp
  ReadH5EbsdTest.cpp
  RodriguesConvertorTest.cpp
  RotateEulerRefFrameTest.cpp
  WritePoleFigureTest.cpp
  WriteStatsGenOdfAngleFileTest.cpp
)
set(DISABLED_TESTS
  CreateLambertSphereTest.cpp
  EMsoftSO3SamplerTest.cpp
  FindDistsToCharactGBsTest.cpp
  FindFeatureReferenceCAxisMisorientationsTest.cpp
  FindTwinBoundariesTest.cpp
  FindTwinBoundarySchmidFactorsTest.cpp
  GenerateOrientationMatrixTransposeTest.cpp
  INLWriterTest.cpp

  # ImportEbsdMontageTest.cpp    # MISSING 1 or more Parameter Implementations
  ReplaceElementAttributesWithNeighborValuesTest.cpp
  Stereographic3DTest.cpp
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
# PUBLIC
# ${CMAKE_CURRENT_LIST_DIR}/../../src
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
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_ImportH5Data.tar.gz SHA512 68cf620d28a515d04040d66c4f8fa9d8d46707c65138b366b47f40d1d56a36c40e2e2cd0c9d35168b68c2cb5b2ce95650fa2a0d4e6ffce980d0dd0654908f40d)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_Small_IN100_GBCD.tar.gz SHA512 543e3bdcee24ff9e5cd80dfdedc39ceef1529e7172cebd01d8e5518292ffdf4e0eb2e79d75854cb3eaca5c60e19c861ca67f369e21b81c306edb66327f47a1e3)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_misorientation.tar.gz SHA512 2343654a8bcb344fcc613b6715c1e0b2c780fedbdf06cc8e5306b23f9d7908d5eef8faff7e3f0dd6f7ac734a1c6e2b376832bed38548288cd5e9e0af1b5602a8)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_caxis_data.tar.gz SHA512 6c3eefac62aecf3344de735c0df932dd9a2623653f39174ec72402c54e8911402ad9903ae2122bc23ff2dba8c94973b2882715908e1913973dff604b614a5293)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_ebsd_segment_features.tar.gz SHA512 acbb493a0668e0115ac49d4fedbbf7600759b9a66deb5d1004c2749a61d2bad2fcc60344bf72b2aeda5c8c098f458949dd1f8d58cb21682fa1393dfb7d0a1b84)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_gbcd_metric_based.tar.gz SHA512 46032c758acc982eccaff38c3252a4f063b6ff5dc9ba3f33ed7f123c6f10771e1a2bdcbff8aab0fae7c91c03bb6025f49f0edbb085b3946b93b87980d31e4543)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_stats_test.tar.gz SHA512 cb1d1c004ab5a3cb29cc10f7b6c291dd5819e57e303242b8162cd0b268dea24b1d3e5e3811ec4f5ee216179b6eb6b81c76ee7d37b7c49e83c6ab336147b4b14e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME PoleFigure_Exemplars.tar.gz SHA512 7c90c69020a4fd4de10da81c1bc87499f5aededca57f3fc1aae3c60aad344cab1bdad2927d957a8d5848aca599028a9ade532dabc784f099bce2de39f61060e9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_Ang_Files.tar.gz SHA512 79e9f6948d4e8e06187e11216a67596fa786ffd2700e51f594ad014090383eb8bcc003e14de2e88082aa9ae512cc4fc9cee22c80066fc54f38c3ebc75267eb5b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_dream3d.tar.gz SHA512 6dd8a3412532bdc7481f7781c7087b4477c6a1efbe6b214f997dad30c53c59714a751be522f084b98065fe75100c74df901bb8af2f512ef47344d8f7941575cf)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_h5ebsd.tar.gz SHA512 31e606285ea9e8235dcb5f608fd2b252a5ab1492abd975e5ec33a21d083aa9720fe16fb8f752742c140f40e963d692f1a46256b9d36e96b1b09796c1e4ea3db9)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME align_sections.tar.gz SHA512 b6892e437df86bd79bd2f1d8f48e44d05bfe38b3453058744320bfaf1b1dc461a484edc9e593f6b9de4ad4d04c41b5dbd0a30e6fc605341d046aec4c3062e33e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME bad_data_neighbor_orientation_check.tar.gz SHA512 6452cfb1f2394c10050082256f60a2068cfad78ef742e9e35b1d6e63b3fb7c35c9fe7bbe093bed4dbb4e758c49ec6da7b1f7e2473838a0421f39fbdd9f4a2f76)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME neighbor_orientation_correlation.tar.gz SHA512 122367452174ade2f24dde7a4610bddc4f147a223722d9b30c1df9eaa2cd2bf25e1c7957aba83f3f9de79b4eadd79339b848f9530d1ebf44c69244ea5442cf85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME so3_cubic_high_ipf_001.tar.gz SHA512 dfe4598cd4406e8b83f244302dc4fe0d4367527835c5ddd6567fe8d8ab3484d5b10ba24a8bb31db269256ec0b5272daa4340eedb5a8b397755541b32dd616b85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME feature_boundary_neighbor_slip_transmission.tar.gz SHA512 fb4a92bdfcff1f2b167cdce2126087e76c9022deb0f0bd8a658482f88f8593bacc0e23b8056d56390f767bc1dd21912fb01a976e3cb670738f1a39d46ad9ef28)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_read_ang_data.tar.gz SHA512 1777431a1623e2fffdc2daad9be51a72c3bdf6a83a33893827892c98a811991e21f1cf636e036604d0bbc523d8ca0b9d655c28be3d0e89ccafc1486dfa0bd0c7)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_read_ctf_data.tar.gz SHA512 e7ac4706d22574396aaa5d67b2a2b4e32db5005e841d3b6dafad65c74615441c028e4cba0ce0721c8929dace495afd6ef844585e2ce6d3b092582a258befd7c2)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME write_stats_gen_odf_angle_file.tar.gz SHA512 be3f663aae1f78e5b789200421534ed9fe293187ec3514796ac8177128b34ded18bb9a98b8e838bb283f9818ac30dc4b19ec379bdd581b1a98eb36d967cdd319)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME INL_writer.tar.gz SHA512 7d723351e51e84540abfbc38e69a6014852ba34808f9d216a27063a616bcfbd5eb708405305fd83334e48c9ca133d3d0be797c05040e4a115cc612e385d9ada6)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME convert_hex_grid_to_square_grid_test.tar.gz SHA512 bb672ebbe2540ba493ad95bea95dac1f85b5634ac3311b5aa774ce3d2177103d1b45a13225221993dd40f0cbe02daf20ccd209d4ae0cab0bf034d97c5b234ba4)

endif()

# -----------------------------------------------------------------------------
# Include any example pipelines into the unit testing
# -----------------------------------------------------------------------------
include(${complex_SOURCE_DIR}/cmake/Utility.cmake)

set(TEST_PIPELINE_LIST_FILE ${${PLUGIN_NAME}_BINARY_DIR}/PrebuiltPipelineTests/nxrunner_tests.txt)
FILE(WRITE ${TEST_PIPELINE_LIST_FILE} "Prebuilt Pipeline Tests\n")
get_target_property(PIPELINE_RUNNER_NAME nxrunner NAME)
get_target_property(PIPELINE_RUNNER_DEBUG nxrunner DEBUG_POSTFIX)

# message(STATUS "PIPELINE_RUNNER_NAME: ${PIPELINE_RUNNER_NAME}")
# message(STATUS "PIPELINE_RUNNER_DEBUG: ${PIPELINE_RUNNER_DEBUG}")

# ------------------------------------------------------------------------------
# This function creates a CTest for a given pipeiline file.
# PLUGIN_NAME: Name of the plugin: Typically you can use ${PLUGIN_NAME}
# PIPELINE_PATH: The absolute path to the pipeline file
# TEST_INDEX: The numerical test index. This can be reset for each plugin
# DELETE_FILE_LIST: The list of files that should be deleted from the system
# after the test is run
# ------------------------------------------------------------------------------
function(add_pipeline_test)
  set(optionsArgs)
  set(oneValueArgs PLUGIN_NAME PIPELINE_PATH TEST_INDEX)
  set(multiValueArgs INPUT_FILE_LIST OUTPUT_FILE_LIST DELETE_FILE_LIST)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(TEST_SCRIPT_FILE_EXT "sh")
  set(EXE_EXT "")
  set(RM_COMMAND "rm")

  if(WIN32)
    set(TEST_SCRIPT_FILE_EXT "bat")
    set(EXE_EXT ".exe")
    set(RM_COMMAND "del")
  endif()

  set(PREBUILT_SUPPORT_DIR "${${PLUGIN_NAME}_SOURCE_DIR}")

  FILE(APPEND ${TEST_PIPELINE_LIST_FILE} "[${ARGS_TEST_INDEX}]:${ARGS_PIPELINE_PATH}\n")

  get_filename_component(test "${ARGS_PIPELINE_PATH}" NAME)
  set(COMPLEX_CTEST_TEST_DRIVER "${${PLUGIN_NAME}_BINARY_DIR}/PrebuiltPipelineTests/${test}.${TEST_SCRIPT_FILE_EXT}")

  # message(STATUS "==> ${COMPLEX_CTEST_TEST_DRIVER}")
  set(DELETE_FILE_COMMANDS "")

  get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
  if(is_multi_config)
    set(CX_CONFIG_DIR "$<CONFIG>")
  else()
    set(CX_CONFIG_DIR ".")
  endif()

  if(WIN32)
    set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/%CONFIG_DIR%/")
  else()
    set(DATA_DEST_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${CX_CONFIG_DIR}/")
  endif()

  foreach(f ${ARGS_DELETE_FILE_LIST})
    set(abs_file_path ${DATA_DEST_DIR}${f})

    if(WIN32)
      file(TO_NATIVE_PATH ${abs_file_path} abs_file_path)
    endif()

    set(DELETE_FILE_COMMANDS "${DELETE_FILE_COMMANDS}echo Deleting Input file '${abs_file_path}'\n${RM_COMMAND} \"${abs_file_path}\"\n")
    FILE(APPEND ${TEST_PIPELINE_LIST_FILE} "    [DELETES FILE]:${abs_file_path}\n")
  endforeach()

  # ------------------------------------------------------------------------------
  # Set up the checks for the input files.
  # ------------------------------------------------------------------------------
  set(INPUT_FILE_COMMANDS "")

  foreach(f ${ARGS_INPUT_FILE_LIST})
    set(abs_file_path ${DATA_DEST_DIR}${f})

    if(WIN32)
      file(TO_NATIVE_PATH ${abs_file_path} abs_file_path)
    endif()

    if(WIN32)
      set(INPUT_FILE_COMMANDS
        "${INPUT_FILE_COMMANDS}echo \"Checking Input file '${abs_file_path}'\"
if exist \"${abs_file_path}\" (
  @echo \"    Input file exists\"
) else (
  @echo \"    Input file does not exist\"
  exit 1
)
")
    else()
      set(INPUT_FILE_COMMANDS
        "${INPUT_FILE_COMMANDS}echo \"Checking Input file '${abs_file_path}'\"
if [ -e \"${abs_file_path}\" ]
then
  echo \"    Input file exists\"
else
  echo \"    Input file does not exist\"
  exit 1
fi
")
    endif()

    # FILE(APPEND ${TEST_PIPELINE_LIST_FILE} "    [DELETES FILE]:${abs_file_path}\n")
  endforeach()

  set(OUTPUT_FILE_COMMANDS "")

  foreach(f ${ARGS_OUTPUT_FILE_LIST})
    set(abs_file_path ${DATA_DEST_DIR}${f})

    if(WIN32)
      file(TO_NATIVE_PATH ${abs_file_path} abs_file_path)
    endif()

    if(WIN32)
      set(OUTPUT_FILE_COMMANDS
        "${OUTPUT_FILE_COMMANDS}echo \"Checking output file '${abs_file_path}'\"
if exist \"${abs_file_path}\" (
  @echo \"    Input file exists\"
) else (
  @echo \"    Input file does not exist\"
  exit 1
)
")
    else()
      set(OUTPUT_FILE_COMMANDS
        "${OUTPUT_FILE_COMMANDS}echo \"Checking output file '${abs_file_path}'\"
if [ -e \"${abs_file_path}\" ]
then
  echo \"    Output file exists\"
else
  echo \"    Output file does not exist\"
  exit 1
fi
")
    endif()

    # FILE(APPEND ${TEST_PIPELINE_LIST_FILE} "    [DELETES FILE]:${abs_file_path}\n")
  endforeach()

  configure_file(${${PLUGIN_NAME}_SOURCE_DIR}/test/ctest_pipeline_driver.${TEST_SCRIPT_FILE_EXT}
    "${COMPLEX_CTEST_TEST_DRIVER}" @ONLY)

  string(REPLACE "/" "_" test "${test}")
  add_test(NAME ${PLUGIN_NAME}_${ARGS_TEST_INDEX} COMMAND "${COMPLEX_CTEST_TEST_DRIVER}")
endfunction()

# ------------------------------------------------------------------------------
# Setup our Prebuilt reconstruction pipelines for the Small IN100 Test Data
# ------------------------------------------------------------------------------
set(test_index 0)
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Reconstruction/(01) Small IN100 Archive.d3dpipeline"
  TEST_INDEX "${test_index}")

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Reconstruction/(08) Small IN100 Full Reconstruction.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/Reconstruction/Small_IN100.h5ebsd"
  OUTPUT_FILE_LIST "Data/Output/Reconstruction/SmallIN100_Final.dream3d" "Data/Output/Reconstruction/SmallIN100_Final.xdmf"
)

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Statistics/(01) Small IN100 Morphological Statistics.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/Reconstruction/SmallIN100_Final.dream3d"
  OUTPUT_FILE_LIST "Data/Output/Statistics/SmallIN100_Morph.dream3d" "Data/Output/Statistics/SmallIN100_Morph.xdmf"
  DELETE_FILE_LIST "Data/Output/Reconstruction/SmallIN100_Final.dream3d" "Data/Output/Reconstruction/SmallIN100_Final.xdmf")

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD Statistics/(05) Small IN100 Crystallographic Statistics.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/Statistics/SmallIN100_Morph.dream3d"
  OUTPUT_FILE_LIST "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d" "Data/Output/Statistics/SmallIN100_CrystalStats.xdmf"
  DELETE_FILE_LIST "Data/Output/Statistics/SmallIN100_Morph.dream3d" "Data/Output/Statistics/SmallIN100_Morph.xdmf")

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(01) Small IN100 Quick Mesh.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
  OUTPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_Mesh.dream3d" "Data/Output/SurfaceMesh/SmallIN100_Mesh.xdmf"
  DELETE_FILE_LIST "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d" "Data/Output/Statistics/SmallIN100_CrystalStats.xdmf")

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(02) Small IN100 Smooth Mesh.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_Mesh.dream3d"
  OUTPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d" "Data/Output/SurfaceMesh/SmallIN100_Smoothed.xdmf"
  DELETE_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_Mesh.dream3d" "Data/Output/SurfaceMesh/SmallIN100_Mesh.xdmf")

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(03) Small IN100 Mesh Statistics.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d"
  OUTPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d" "Data/Output/SurfaceMesh/SmallIN100_MeshStats.xdmf"
  DELETE_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d" "Data/Output/SurfaceMesh/SmallIN100_Smoothed.xdmf")

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(04) Small IN100 GBCD.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
  OUTPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_GBCD.dream3d" "Data/Output/SurfaceMesh/SmallIN100_GBCD.xdmf"
  DELETE_FILE_LIST "Data/Output/SmallIN100GBCD/SmallIn100Triangles.ph"
  "Data/Output/SmallIN100GBCD/SmallIn100GMT_1.dat"
)

math(EXPR test_index "${test_index} + 1")
add_pipeline_test(PLUGIN_NAME ${PLUGIN_NAME}
  PIPELINE_PATH "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EBSD SurfaceMeshing/(05) Small IN100 GBCD Metric.d3dpipeline"
  TEST_INDEX "${test_index}"
  INPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
  OUTPUT_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_GBCD_Metric.dream3d" "Data/Output/SurfaceMesh/SmallIN100_GBCD_Metric.xdmf"
  DELETE_FILE_LIST "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
  "Data/Output/SurfaceMesh/SmallIN100_MeshStats.xdmf"
  "Data/Output/SurfaceMesh/7_0_small_in100_distribution_1.dat"
  "Data/Output/SurfaceMesh/7_0_small_in100_distribution_errors_1.dat"
  "Data/Output/SurfaceMesh/SmallIN100_GBCD_Metric.dream3d"
)

# These should be relative to the
set(PREBUILT_PIPELINE_NAMES

  # Example or other test pipelines
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/AlignSectionsMutualInformation.d3dpipeline" # This example depends on the SmallIN100.h5ebsd file
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/CI_Histogram.d3dpipeline"
  "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/EnsembleInfoReader.d3dpipeline"
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
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/aptr12_Analysis.d3dpipeline"
    "${${PLUGIN_NAME}_SOURCE_DIR}/pipelines/avtr12_Analysis.d3dpipeline"
  )
endif()

create_pipeline_tests(PLUGIN_NAME ${PLUGIN_NAME} PIPELINE_LIST ${PREBUILT_PIPELINE_NAMES})
