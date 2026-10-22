
#==================================================================================================
# Define these variables
#==================================================================================================
# get_filename_component(HOST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
## This is the Fully Qualified host name of your system
set(CTEST_SITE "$ENV{AGENT_NAME}.bluequartz.net")
## The name for this build. Combine the build type, compiler, and OS into a single string
include("${CMAKE_CURRENT_LIST_DIR}/azure_ci_mode.cmake")
set(CTEST_BUILD_NAME "${CMAKE_PRESET_NAME}-$ENV{BUILD_BUILDNUMBER}-PR$ENV{SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}")
## The type of build we are going to do "Release | Debug"
#set(CTEST_CONFIGURATION_TYPE Release)
## The type of generator we are going to use "Make | Ninja | NMake | JOM"
set(CTEST_CMAKE_GENERATOR "Ninja")

#==================================================================================================
# Look for a GIT command-line client.
#==================================================================================================
if(NOT DEFINED CTEST_GIT_COMMAND)
  find_program(CTEST_GIT_COMMAND NAMES git git.exe git.cmd)
endif()

if(NOT EXISTS "${CTEST_GIT_COMMAND}")
  message(FATAL_ERROR "CONFIGURE ERRORS: Git is required to record the Azure-selected commit.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  No Git Found.")
endif()


#==================================================================================================
# Configure the project
#==================================================================================================
file(WRITE "${CTEST_BINARY_DIRECTORY}/CMakeCache.txt" "
      SITE:STRING=${CTEST_SITE}
      BUILDNAME:STRING=${CTEST_BUILD_NAME}
      CMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE}
      DART_TESTING_TIMEOUT:STRING=1500"
)

#==================================================================================================
# Start CTest the project. This will create the TAG that is APPENDED to for the other 2 steps in
# the process.
#==================================================================================================
ctest_start(Experimental "${CTEST_SOURCE_DIR}" "${CTEST_BINARY_DIRECTORY}")

#==================================================================================================
# Record the Git hash for the project
#==================================================================================================
# Azure has already selected the commit. Record it without fetching or pulling.
execute_process(COMMAND "${CTEST_GIT_COMMAND}" -C "${CTEST_SOURCE_DIR}" rev-parse HEAD
  RESULT_VARIABLE git_result OUTPUT_VARIABLE source_hash OUTPUT_STRIP_TRAILING_WHITESPACE)
if(NOT "${git_result}" STREQUAL "0")
  message(FATAL_ERROR "Cannot read the Azure-selected commit for simplnx: ${git_result}")
endif()
message(STATUS "Azure-selected simplnx: ${source_hash}")
file(WRITE "${CTEST_BINARY_DIRECTORY}/CISourceVersions.txt" "simplnx: ${source_hash}\n")


#==================================================================================================
# Configure the project
#==================================================================================================
ctest_configure(  BUILD "${CTEST_BINARY_DIRECTORY}"
                  SOURCE "${CTEST_SOURCE_DIR}"
                  OPTIONS "${ci_configure_options}"
                  RETURN_VALUE ctest_configure_result
                  CAPTURE_CMAKE_ERROR ctest_cmake_result)

message(STATUS "ctest_configure_result: ${ctest_configure_result}")
message(STATUS "    ctest_cmake_result: ${ctest_cmake_result}")

if(NOT "${ctest_cmake_result}" STREQUAL "0" OR NOT "${ctest_configure_result}" STREQUAL "0")
  message(FATAL_ERROR "CONFIGURE ERRORS: See the ctest output above in this step's log.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  CMake returned the following error code during configure: ${ctest_cmake_result}
                  ctest_configure() returned the following error code during configure: ${ctest_configure_result}")
endif()
ci_validate_cache()
