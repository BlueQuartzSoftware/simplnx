include("${simplnx_SOURCE_DIR}/cmake/Plugin.cmake")

set(PLUGIN_NAME "TestTwo")
set(${PLUGIN_NAME}_SOURCE_DIR ${simplnx_SOURCE_DIR}/src/Plugins/${PLUGIN_NAME})

#------------------------------------------------------------------------------
# These are all the filters in the plugin. All filters should be kept in the
# PLUGIN_NAME/src/PLUGIN_NAME/Filters/ directory.
set(FilterList
  Test2Filter
)

create_simplnx_plugin(NAME ${PLUGIN_NAME}
                      FILTER_LIST ${FilterList}
                      DESCRIPTION "Test Plugin for ${PLUGIN_NAME}"
                      VERSION "0.1.0"
)

#------------------------------------------------------------------------------
# If there are additional library packages that need to be found, there is where
# to do that using the usual find_package(....) call
# find_package(....)


#------------------------------------------------------------------------------
# If there are additional libraries that this plugin needs to link against you
# can use the target_link_libraries() cmake call
# target_link_libraries(${PLUGIN_NAME} PUBLIC [name of library])

#------------------------------------------------------------------------------
# If there are additional source files that need to be compiled for this plugin
# can use the target_sources(...) cmake call
# target_sources(${PLUGIN_NAME}
#   PUBLIC
#     additional header files
#   PRIVATE
#     additional source files
# )

#------------------------------------------------------------------------------
# If there are additional include directories that are needed for this plugin
# you can use the target_include_directories(.....) cmake call
# target_include_directories(${PLUGIN_NAME}
#     PUBLIC
#     additional include directories here
# )
