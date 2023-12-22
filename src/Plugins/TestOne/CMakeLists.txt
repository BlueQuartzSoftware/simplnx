include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "TestOne")
set(${PLUGIN_NAME}_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# PLUGIN_NAME/src/PLUGIN_NAME/Filters/ directory.
set(FilterList
  ErrorWarningFilter
  ExampleFilter1
  ExampleFilter2
  # CreateOutOfCoreArray
  TestFilter
  DynamicTableExample
)

create_simplnx_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      DESCRIPTION "Test Plugin for ${PLUGIN_NAME}"
                      VERSION "0.1.0"
)

#------------------------------------------------------------------------------
# Add in the unit tests for the filters included in this plugin
if(SIMPLNX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(${${PLUGIN_NAME}_SOURCE_DIR}/test)
endif()
