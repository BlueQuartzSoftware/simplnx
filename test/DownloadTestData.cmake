# -----------------------------------------------------------------------
# Download the Test Data. This needs to be done BEFORE the Plugins and test
# -----------------------------------------------------------------------
file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR)
# If DREAM3D_DATA_DIR is NOT defined/set then take a look at the same level as complex
if("${DREAM3D_DATA_DIR}" STREQUAL "")
  message(STATUS "DREAM3D_DATA_DIR is empty. Attempting to locate DREAM3D_Data at same level as complex directory.")
  get_filename_component(complex_PARENT ${complex_SOURCE_DIR} DIRECTORY CACHE)
  if(EXISTS "${complex_PARENT}/DREAM3D_Data")
    message(STATUS "DREAM3D_Data directory was *found* at same level as `complex`")
    set(DREAM3D_DATA_DIR "${complex_PARENT}/DREAM3D_Data" CACHE PATH "The directory where to find test data files")
  else()
    message(WARNING "DREAM3D_Data directory was *not* found at the same level as `complex` source. Setting it to '${complex_BINARY_DIR}/DREAM3D_Data'")
    set(DREAM3D_DATA_DIR "${complex_BINARY_DIR}/DREAM3D_Data" CACHE PATH "The directory where to find test data files")
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}/TestFiles/")
  endif()
endif()

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
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_feature_centroid.tar.gz SHA512 e79a4c8e59bc856d40e91daf4cdce8b82c2e5ccfa6de51e23a0b8c6628282b03701bd5b5d7ddde76c4378142e3d9fe7c1cd6db261360c91751cf7c63747b054b)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_align_sections_misorientation.tar.gz SHA512 2343654a8bcb344fcc613b6715c1e0b2c780fedbdf06cc8e5306b23f9d7908d5eef8faff7e3f0dd6f7ac734a1c6e2b376832bed38548288cd5e9e0af1b5602a8)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_ebsd_segment_features.tar.gz SHA512 acbb493a0668e0115ac49d4fedbbf7600759b9a66deb5d1004c2749a61d2bad2fcc60344bf72b2aeda5c8c098f458949dd1f8d58cb21682fa1393dfb7d0a1b84)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_erode_dilate_bad_data.tar.gz SHA512 95556a7a09f7ef85cc21b8e5cb78b15b0edd75791b3d205ec57fd626a7933e72f3f3b6dd07707f28df4d8eb7a35be53818974c38b21ef323f655c29805b335d0)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_feature_centroids.tar.gz SHA512 1eecd4eda617b6b13f3321b33d9cc35fd53f01cf19384c8922318639e86b765d381bd0a07d4dae14628c99cc0e1caffcf23aa667683732d3925307409bda9c28)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_find_shapes.tar.gz SHA512 657bb5015e731afad605274f01f38fa375b423a91159946c691361e69580d282909072fb026a778b09c0f2b6e2359549a4f138072b343a35a2c97170c1af1292)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_input.tar.gz SHA512 f59d0a5c3b414ed11419a1cf1594f430d76a9b462086592a3c6bbf41b4c62b33d571981b9fc277e4c6ce43e0a870c02ec9e90d91b49720c8005b87ccbdb11a14)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_min_size_output.tar.gz SHA512 2ad1bc5e99eddf9939b52e16fcd1d8ca70ec37bfe9396b7123af3d8f11f83d015cf0bae2b5e903e65cec8e5d7bda328649042253a046d1e6c89b637e0a5faf91)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_stats_test.tar.gz SHA512 cb1d1c004ab5a3cb29cc10f7b6c291dd5819e57e303242b8162cd0b268dea24b1d3e5e3811ec4f5ee216179b6eb6b81c76ee7d37b7c49e83c6ab336147b4b14e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME 6_6_volume_fraction_feature_count.dream3d.tar.gz SHA512 e4205186dee5b97bef3fb5f265bc12f8379a68860f70de33a483680eba0433a35c9c98db56bc92c949fb215f73f5f7698f83394697e81604c8c4b8a06874943f)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ASCII_Data.tar.gz SHA512 4696b919510aa400b9de6c72d0abe66e7c8be2a4effd350bfd07c4fc24d25ccb45c22e7aca5ad162d88db3e04d700b86f7d1d7e954db90ef00b31b5b46955ed4)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Example.tar.gz SHA512 b7ea2b8bb5ecb29cb20d69e5820ebf513433c7740ce5cc9e6c1e44b539941dd3cf1a627442ace2cd430fe64437a479a62a172e6d398edf992ccdfc906ebceeba)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME ResampleImageGeom_Exemplar.tar.gz SHA512 464029b7354b96a943d75c495ef02bac0f834032e5a86576dde9afee51febff3fd6ffd7d4f8f1e9f8315d8cda36971df26601c7212e1876151109ca5428b8659)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME SmallIN100.tar.gz SHA512 31a32ba5d1296ef184c2134fe14023c20a5e5c21b95b3f4193c4394db19c86b241cd54d31b0e0128d40c0bfbd6460657531094e588b5d4d224485d17f7c3ed36)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100.tar.gz SHA512 da6c5ea94b90a1877c41fbaf7390bae9230f969aa4b953cf39b6deff8fb529ba1a299e2f13b59ec2835dc1f524514342a21aebc55916e92d91c098723412ea24)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME Small_IN100_Exemplar.tar.gz SHA512 c57684c31f73fe1e5871ffd760a8c1af9498c86c4e5bcec09997e89126d1eb6ef85bf96d32a60992de44cba510839f257f5d73585e8413ce34ce65e3b6f433d5)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME align_sections.tar.gz SHA512 b6892e437df86bd79bd2f1d8f48e44d05bfe38b3453058744320bfaf1b1dc461a484edc9e593f6b9de4ad4d04c41b5dbd0a30e6fc605341d046aec4c3062e33e)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME bad_data_neighbor_orientation_check.tar.gz SHA512 6452cfb1f2394c10050082256f60a2068cfad78ef742e9e35b1d6e63b3fb7c35c9fe7bbe093bed4dbb4e758c49ec6da7b1f7e2473838a0421f39fbdd9f4a2f76)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME neighbor_orientation_correlation.tar.gz SHA512 122367452174ade2f24dde7a4610bddc4f147a223722d9b30c1df9eaa2cd2bf25e1c7957aba83f3f9de79b4eadd79339b848f9530d1ebf44c69244ea5442cf85)
  download_test_data(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR} ARCHIVE_NAME so3_cubic_high_ipf_001.tar.gz SHA512 dfe4598cd4406e8b83f244302dc4fe0d4367527835c5ddd6567fe8d8ab3484d5b10ba24a8bb31db269256ec0b5272daa4340eedb5a8b397755541b32dd616b85)  
  

endif()

# -----------------------------------------------------------------------------
# Copy the user facing data files to the binary directory
# -----------------------------------------------------------------------------
if(EXISTS "${DREAM3D_DATA_DIR}")
    create_data_copy_rules(DREAM3D_DATA_DIR ${DREAM3D_DATA_DIR})
endif()
