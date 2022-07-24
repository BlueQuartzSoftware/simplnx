# -----------------------------------------------------------------------
# Download the Test Data
# -----------------------------------------------------------------------
include(ExternalProject)
include(download_dir)

ExternalProject_Add(TestFiles
  URL  https://github.com/dream3d/DREAM3D_Data/releases/download/v6_4/TestFiles.tar.gz

  DOWNLOAD_DIR ${DREAM3D_DATA_DIR}

  CONFIGURE_COMMAND "" 
  BUILD_COMMAND "" 
  INSTALL_COMMAND ""

  LOG_DOWNLOAD 1
  LOG_UPDATE 1
  LOG_CONFIGURE 1
  LOG_BUILD 1
  LOG_TEST 1
  LOG_INSTALL 1
)




# cmake_minimum_required(VERSION 3.19)

# string(TIMESTAMP complex_plugins_VERSION "%Y.%m.%d")

# project(complex_plugins
#   VERSION ${complex_plugins_VERSION}
#   DESCRIPTION "complex_plugins"
#   HOMEPAGE_URL "https://github.com/BlueQuartzSoftware/complex_plugins"
#   LANGUAGES CXX
# )

# # --------------------------------------------------------------------------------------------------
# # Find the parent directory for this project
# # --------------------------------------------------------------------------------------------------
# get_filename_component(parent_dir "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY CACHE)
# include("${parent_dir}/complex")
