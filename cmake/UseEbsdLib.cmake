# ------------------------------------------------------------------------------
# Required EbsdLib and H5Support
# ------------------------------------------------------------------------------

if(SIMPLNX_BUILD_EBSDLIB)
  find_package(H5Support REQUIRED)
  find_package(EbsdLib REQUIRED)
else()

    if(NOT TARGET EbsdLib::EbsdLib)

        if(EXISTS "${simplnx_SOURCE_DIR}/../EbsdLib")
            set(EbsdLibProj_SOURCE_DIR "${simplnx_SOURCE_DIR}/../EbsdLib")
        else()
            message(FATAL_ERROR "EbsdLibProj_SOURCE_DIR was not set. Where is the EbsdLib project directory. Please set the EbsdLibProj_SOURCE_DIR variable to the EbsdLib directory.")
        endif()
        message(STATUS "EbsdLibProj_SOURCE_DIR: ${EbsdLibProj_SOURCE_DIR}")

        set(EbsdLib_ENABLE_HDF5 ON)
        set(EbsdLib_USE_PARALLEL_ALGORITHMS ${SIMPLNX_ENABLE_MULTICORE})
        set(EbsdLib_BUILD_H5SUPPORT ON)
        set(H5Support_INCLUDE_QT_API OFF)
        add_subdirectory( ${EbsdLibProj_SOURCE_DIR} ${PROJECT_BINARY_DIR}/EbsdLib)
    endif()
endif()

