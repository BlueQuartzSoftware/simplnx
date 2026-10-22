
#==================================================================================================
# Define these variables
#==================================================================================================
# get_filename_component(HOST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
## This is the Fully Qualified host name of your system
set(CTEST_SITE "$ENV{AGENT_NAME}.bluequartz.net")
## The name for this build. Combine the build type, compiler, and OS into a single string
include("${CMAKE_CURRENT_LIST_DIR}/azure_ci_mode.cmake")
ci_validate_cache()
if(NOT DEFINED CTEST_TEST_TIMEOUT)
  load_cache("${CTEST_BINARY_DIRECTORY}" READ_WITH_PREFIX ci_cache_ DART_TESTING_TIMEOUT)
  if(DEFINED ci_cache_DART_TESTING_TIMEOUT AND NOT "${ci_cache_DART_TESTING_TIMEOUT}" STREQUAL "")
    set(CTEST_TEST_TIMEOUT "${ci_cache_DART_TESTING_TIMEOUT}")
  endif()
endif()
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
# Run the unit tests serially
#==================================================================================================
# In ctest -S mode, capture test failures and report a fatal error to set the process exit code.
# ctest_test writes Test.xml, which the pipeline PublishTestResults task publishes to Azure.
ctest_test(APPEND PARALLEL_LEVEL 1
  RETURN_VALUE ctest_test_result CAPTURE_CMAKE_ERROR ctest_cmake_result)
message(STATUS "ctest_test_result: ${ctest_test_result}")
message(STATUS "ctest_cmake_result: ${ctest_cmake_result}")

if(NOT "${ctest_test_result}" STREQUAL "0" OR NOT "${ctest_cmake_result}" STREQUAL "0")
  message(FATAL_ERROR "TEST FAILURES: See the failed tests above in this step's log and in the build's Tests tab.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  CMake returned the following error code during testing: ${ctest_cmake_result}
                  ctest_test() returned the following error code during testing: ${ctest_test_result}")
endif()
