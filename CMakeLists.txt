# -----------------------------------------------------------------------
# Download the Test Data
# -----------------------------------------------------------------------
include(ExternalProject)
include(download_dir)

ExternalProject_Add(TestFiles
  URL https://github.com/dream3d/DREAM3D_Data/releases/download/v6_5/TestFiles_6_5.tar.gz
  URL_HASH SHA512=a8e92701cb47d8d66311c04ad86bbaa6ced45b9c9167b8fc2780e4030db7ad82223c011901a3609a33edb60ebed0962dc49f465ed86c8d8a17b398c9ab264afd
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
