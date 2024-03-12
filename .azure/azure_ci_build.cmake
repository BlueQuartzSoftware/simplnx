
#==================================================================================================
# Define these variables
#==================================================================================================
# get_filename_component(HOST_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)
## This is the Fully Qualified host name of your system
set(CTEST_SITE "$ENV{AGENT_NAME}.bluequartz.net")
## The name for this build. Combine the build type, compiler, and OS into a single string
set(CTEST_BUILD_NAME "$ENV{PRESET_NAME}-$ENV{BUILD_BUILDNUMBER}-PR$ENV{SYSTEM_PULLREQUEST_PULLREQUESTNUMBER}")
## The type of build we are going to do "Release | Debug"
#set(CTEST_CONFIGURATION_TYPE Release)
## The type of generator we are going to use "Make | Ninja | NMake | JOM"
set(CTEST_CMAKE_GENERATOR "Ninja")

#==================================================================================================
# Append to the existing CTEST TAG
#==================================================================================================
ctest_start(Experimental ${CTEST_SOURCE_DIR} ${CTEST_BINARY_DIRECTORY} APPEND)

#==================================================================================================
# Build the project
#==================================================================================================
ctest_build(BUILD ${CTEST_BINARY_DIRECTORY}
            CONFIGURATION ${CTEST_CONFIGURATION_TYPE}
            NUMBER_ERRORS ctest_build_errors
            NUMBER_WARNINGS ctest_build_warnings
            RETURN_VALUE ctest_build_result
            CAPTURE_CMAKE_ERROR ctest_cmake_result
            )
ctest_submit(PARTS Build)

if("${ctest_build_result}" EQUAL -1 OR "${ctest_cmake_result}" EQUAL -1)
  message(FATAL_ERROR "CONFIGURE ERRORS: Go to https://my.cdash.org/index.php?project=DREAM3D for more information.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  CMake returned the following error code during build: ${ctest_cmake_result}
                  ctest_build() returned the following error code during build: ${ctest_build_result}")
endif()

