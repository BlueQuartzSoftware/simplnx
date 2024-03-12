
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
# Look for a GIT command-line client.
#==================================================================================================
if(NOT DEFINED CTEST_GIT_COMMAND)
  find_program(CTEST_GIT_COMMAND NAMES git git.exe git.cmd)
endif()

if(NOT EXISTS ${CTEST_GIT_COMMAND})
  message(FATAL_ERROR "CONFIGURE ERRORS: Go to https://my.cdash.org/index.php?project=DREAM3D for more information.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  No Git Found.")
endif()


#==================================================================================================
# Configure the project
#==================================================================================================
file(WRITE ${CTEST_BINARY_DIRECTORY}/CMakeCache.txt "
      SITE:STRING=${CTEST_SITE}
      BUILDNAME:STRING=${CTEST_BUILD_NAME}
      CTEST_USE_LAUNCHERS:BOOL=${CTEST_USE_LAUNCHERS}
      DART_TESTING_TIMEOUT:STRING=${CTEST_TEST_TIMEOUT}
      CMAKE_BUILD_TYPE:STRING=${CTEST_CONFIGURATION_TYPE}
      DART_TESTING_TIMEOUT:STRING=1500"
)

#==================================================================================================
# Start CTest the project. This will create the TAG that is APPENEDED to for the other 2 steps in
# the process.
#==================================================================================================
ctest_start(Experimental ${CTEST_SOURCE_DIR} ${CTEST_BINARY_DIRECTORY})

#==================================================================================================
# Update Git Hashes for the project
#==================================================================================================
set(REPO_NAMES "s")
foreach(p ${REPO_NAMES})
  ctest_update(SOURCE ${CTEST_DASHBOARD_ROOT}/${p} 
              RETURN_VALUE ctest_update_result
              CAPTURE_CMAKE_ERROR ctest_cmake_result)        
endforeach(p ${REPO_NAMES})
ctest_submit(PARTS Update)


#==================================================================================================
# Configure the project
#==================================================================================================
ctest_configure(  BUILD ${CTEST_BINARY_DIRECTORY}
                  SOURCE ${CTEST_SOURCE_DIR}
                  OPTIONS "--preset ${CMAKE_PRESET_NAME}"
                  RETURN_VALUE ctest_configure_result
                  CAPTURE_CMAKE_ERROR ctest_cmake_result)

ctest_submit(PARTS Configure Notes)
message(STATUS "ctest_configure_result: ${ctest_configure_result}")
message(STATUS "    ctest_cmake_restult: ${ctest_cmake_result}")

if("${ctest_cmake_result}" EQUAL -1 OR "${ctest_configure_result}" EQUAL -1)
  message(FATAL_ERROR "CONFIGURE ERRORS: Go to https://my.cdash.org/index.php?project=DREAM3D for more information.
                  Site: ${CTEST_SITE}
                  Build Name: ${CTEST_BUILD_NAME}
                  CMake returned the following error code during build: ${ctest_cmake_result}
                  ctest_configure() returned the following error code during build: ${ctest_configure_result}")
endif()
