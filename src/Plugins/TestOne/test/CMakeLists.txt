#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, this is where
# to do that using the usual find_package(....) call
# find_package(....)

#------------------------------------------------------------------------------
# Define the list of unit test source files
set(${PLUGIN_NAME}UnitTest_SRCS
  ErrorWarningFilterTest.cpp
)

create_simplnx_plugin_unit_test(PLUGIN_NAME ${PLUGIN_NAME}
                                FILTER_LIST ${${PLUGIN_NAME}UnitTest_SRCS})


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME}UnitTest PUBLIC [name of library])

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}UnitTest
#   PUBLIC
#     additional header files
#   PRIVATE
#     additional source files
# )

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
 target_include_directories(${PLUGIN_NAME}UnitTest
     PUBLIC
     ${simplnx_SOURCE_DIR}/test
 )

target_sources(${PLUGIN_NAME}UnitTest PRIVATE "${simplnx_SOURCE_DIR}/test/ProfileIsolationContract.cpp")

add_test(NAME "${PLUGIN_NAME}_test_profile_isolation"
  COMMAND ${CMAKE_COMMAND}
    "-DPROFILE_TEST_EXECUTABLE=$<TARGET_FILE:${PLUGIN_NAME}UnitTest>"
    "-DPROFILE_TEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/ProfileIsolation"
    -DPROFILE_RUNNER_ONLY=TRUE
    -P "${simplnx_SOURCE_DIR}/test/VerifyTestProfileLifecycle.cmake")
set_tests_properties("${PLUGIN_NAME}_test_profile_isolation" PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
