
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
# Run the unit tests
#==================================================================================================
ctest_test(${CTEST_TEST_ARGS} APPEND)
ctest_submit(PARTS Test)
