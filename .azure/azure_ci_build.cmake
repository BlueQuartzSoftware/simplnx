
#==================================================================================================
# Define these variables
#==================================================================================================
# get_filename_component(HOST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
## This is the Fully Qualified host name of your system
set(CTEST_SITE "$ENV{AGENT_NAME}.bluequartz.net")
## The name for this build. Combine the build type, compiler, and OS into a single string
include("${CMAKE_CURRENT_LIST_DIR}/azure_ci_mode.cmake")
ci_validate_cache()
set(CTEST_BUILD_NAME "${CMAKE_PRESET_NAME}-$ENV{BUILD_BUILDNUMBER}-PR$ENV{SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}")
## The type of build we are going to do "Release | Debug"
#set(CTEST_CONFIGURATION_TYPE Release)
## The type of generator we are going to use "Make | Ninja | NMake | JOM"
set(CTEST_CMAKE_GENERATOR "Ninja")

#==================================================================================================
# Append to the existing CTEST TAG
#==================================================================================================
ctest_start(Experimental "${CTEST_SOURCE_DIR}" "${CTEST_BINARY_DIRECTORY}" APPEND)

#==================================================================================================
# Build the project
#==================================================================================================
ctest_build(BUILD "${CTEST_BINARY_DIRECTORY}"
            CONFIGURATION "${CTEST_CONFIGURATION_TYPE}"
            NUMBER_ERRORS ctest_build_errors
            NUMBER_WARNINGS ctest_build_warnings
            RETURN_VALUE ctest_build_result
            CAPTURE_CMAKE_ERROR ctest_cmake_result
            )

if(NOT "${ctest_build_result}" STREQUAL "0" OR NOT "${ctest_cmake_result}" STREQUAL "0" OR ctest_build_errors GREATER 0)
  message(FATAL_ERROR "BUILD ERRORS: See the ctest output above in this step's log.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  CMake returned the following error code during build: ${ctest_cmake_result}
                  ctest_build() returned the following error code during build: ${ctest_build_result}")
endif()

