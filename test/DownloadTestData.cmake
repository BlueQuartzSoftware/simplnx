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

endif()

