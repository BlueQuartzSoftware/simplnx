function(enable_vcpkg_manifest_feature)
  set(optionsArgs)
  set(oneValueArgs TEST_VAR FEATURE)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(${ARGS_TEST_VAR})
    if(NOT ${ARGS_FEATURE} IN_LIST VCPKG_MANIFEST_FEATURES)
      set(VCPKG_MANIFEST_FEATURES ${VCPKG_MANIFEST_FEATURES} ${ARGS_FEATURE} PARENT_SCOPE)
    endif()
  endif()
endfunction()

function(install_with_directory)
  set(optionsArgs)
  set(oneValueArgs DESTINATION COMPONENT BASE_DIR)
  set(multiValueArgs FILES)
  cmake_parse_arguments(ARGS "${optionsArgs}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  foreach(file ${ARGS_FILES})
      file(RELATIVE_PATH relative_file ${ARGS_BASE_DIR} ${file})
      get_filename_component(destination_dir ${relative_file} DIRECTORY)
      install(FILES ${file}
        DESTINATION ${ARGS_DESTINATION}/${destination_dir}
        COMPONENT ${ARGS_COMPONENT}
      )
  endforeach()
endfunction()

# --------------------------------------------------------------------
# This function optionally compiles a named plugin when compiling sandbox
# This function will add in an Option "complex_ENABLE_${NAME} which
# the programmer can use to enable/disable the compiling of specific plugins
# Arguments:
# PLUGIN_NAME The name of the Plugin
# PLUGIN_SOURCE_DIR The source directory for the plugin
function(complex_COMPILE_PLUGIN)
  set(options)
  set(oneValueArgs PLUGIN_NAME PLUGIN_SOURCE_DIR)
  cmake_parse_arguments(P "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  option(complex_ENABLE_${P_PLUGIN_NAME} "Build the ${P_PLUGIN_NAME}" ON)

  if(complex_ENABLE_${P_PLUGIN_NAME})
    add_subdirectory(${P_PLUGIN_SOURCE_DIR} ${PROJECT_BINARY_DIR}/Plugins/${P_PLUGIN_NAME})
    get_property(PluginNumFilters GLOBAL PROPERTY ${P_PLUGIN_NAME}_filter_count)

    message(STATUS "${P_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")
    #- Now set up the dependency between the main application and each of the plugins so that
    #- things like Visual Studio are forced to rebuild the plugins when launching
    #- the sandbox application
    if(complex_ENABLE_${P_PLUGIN_NAME} AND TARGET complex::complex AND TARGET ${P_PLUGIN_NAME})
      add_dependencies(${P_PLUGIN_NAME} complex::complex )
    endif()

  else()
    message(STATUS "${P_PLUGIN_NAME} [DISABLED]: Use -Dcomplex_ENABLE_${P_PLUGIN_NAME}=ON to Enable Plugin")
  endif()
endfunction()

# -----------------------------------------------------------------------------
# This function will search through the COMPLEX_PLUGIN_SEARCH_DIRS variable and
# look for a directory with the name that holds a CMakeLists.txt file.
# Arguments:
# PLUGIN_NAME The name of the Plugin
#
function(complex_add_plugin)
  set(options)
  set(oneValueArgs PLUGIN_NAME)
  cmake_parse_arguments(P "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  #message(STATUS "complex_add_plugin: ${P_PLUGIN_NAME}")
  #message(STATUS "complex_${P_PLUGIN_NAME}_SOURCE_DIR: ${complex_${P_PLUGIN_NAME}_SOURCE_DIR}")
  if(NOT DEFINED complex_${P_PLUGIN_NAME}_SOURCE_DIR OR NOT EXISTS "${complex_${P_PLUGIN_NAME}_SOURCE_DIR}")
    #message(STATUS "complex_${P_PLUGIN_NAME}_SOURCE_DIR was NOT Defined. Searching for Plugins in 'COMPLEX_PLUGIN_SEARCH_DIRS'")
    foreach(pluginSearchDir ${COMPLEX_PLUGIN_SEARCH_DIRS})
      #message(STATUS "|-Searching:${pluginSearchDir} ")
      if(EXISTS ${pluginSearchDir}/${P_PLUGIN_NAME}/CMakeLists.txt)
        set(complex_${P_PLUGIN_NAME}_SOURCE_DIR ${pluginSearchDir}/${P_PLUGIN_NAME} CACHE PATH "")
        #message(STATUS "  |-Plugin: Defining complex_${P_PLUGIN_NAME}_SOURCE_DIR to ${complex_${P_PLUGIN_NAME}_SOURCE_DIR}")
        break()
      endif()
    endforeach()

    #message(STATUS "${complex_SOURCE_DIR}/../complex_plugins/${P_PLUGIN_NAME}/CMakeLists.txt")
    if(EXISTS ${complex_SOURCE_DIR}/../complex_plugins/${P_PLUGIN_NAME}/CMakeLists.txt)
      set(complex_${P_PLUGIN_NAME}_SOURCE_DIR ${complex_SOURCE_DIR}/../complex_plugins/${P_PLUGIN_NAME} CACHE PATH "")
      #message(STATUS "  |-Plugin: Defining complex_${P_PLUGIN_NAME}_SOURCE_DIR to ${complex_${P_PLUGIN_NAME}_SOURCE_DIR}")
    endif()

  endif()

  # Mark these variables as advanced
  mark_as_advanced(complex_${P_PLUGIN_NAME}_SOURCE_DIR)


  # Now that we have defined where the user's plugin directory is at we
  # need to make sure it has a CMakeLists.txt file in it
  if(EXISTS ${complex_${P_PLUGIN_NAME}_SOURCE_DIR}/CMakeLists.txt)
    set(${P_PLUGIN_NAME}_IMPORT_FILE complex_${P_PLUGIN_NAME}_SOURCE_DIR/CMakeLists.txt)
  endif()


  # By this point we should have everything defined and ready to go...
  if(DEFINED complex_${P_PLUGIN_NAME}_SOURCE_DIR AND DEFINED ${P_PLUGIN_NAME}_IMPORT_FILE)
      #message(STATUS "Plugin: Adding Plugin ${complex_${P_PLUGIN_NAME}_SOURCE_DIR}")
      complex_COMPILE_PLUGIN(PLUGIN_NAME ${P_PLUGIN_NAME}
                             PLUGIN_SOURCE_DIR ${complex_${P_PLUGIN_NAME}_SOURCE_DIR})
  else()
      set(complex_${P_PLUGIN_NAME}_SOURCE_DIR ${pluginSearchDir} CACHE PATH "" FORCE)
      message(STATUS "${P_PLUGIN_NAME} [DISABLED]. Missing Source Directory. Use -Dcomplex_${P_PLUGIN_NAME}_SOURCE_DIR=/Path/To/PluginDir")
  endif()


endfunction()

