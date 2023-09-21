include(GenerateExportHeader)
include(GNUInstallDirs)

# --------------------------------------------------------------------
# This function optionally compiles a named plugin when compiling sandbox
# This function will add in an Option "COMPLEX_PLUGIN_ENABLE_${NAME} which
# the programmer can use to enable/disable the compiling of specific plugins
# Arguments:
# PLUGIN_NAME The name of the Plugin
# PLUGIN_SOURCE_DIR The source directory for the plugin
function(complex_COMPILE_PLUGIN)
  set(options)
  set(oneValueArgs PLUGIN_NAME PLUGIN_SOURCE_DIR DOC_CHECK)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  option(COMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME} "Build the ${ARGS_PLUGIN_NAME}" ON)

  if(COMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME})
    add_subdirectory(${ARGS_PLUGIN_SOURCE_DIR} ${PROJECT_BINARY_DIR}/Plugins/${ARGS_PLUGIN_NAME})
    get_property(PluginNumFilters GLOBAL PROPERTY ${ARGS_PLUGIN_NAME}_filter_count)
    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/Plugins/${ARGS_PLUGIN_NAME}/test_output)
    # message(STATUS "${ARGS_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")

    #- Now set up the dependency between the main application and each of the plugins so that
    #- things like Visual Studio are forced to rebuild the plugins when launching
    #- the sandbox application
    if(COMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME} AND TARGET complex::complex AND TARGET ${ARGS_PLUGIN_NAME})
      add_dependencies(${ARGS_PLUGIN_NAME} complex::complex)
    endif()

  else()
    message(STATUS "${ARGS_PLUGIN_NAME} [DISABLED]: Use -DCOMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME}=ON to Enable Plugin")
  endif()
endfunction()

# -----------------------------------------------------------------------------
# This function will search through the COMPLEX_PLUGIN_SEARCH_DIRS variable and
# look for a directory with the name that holds a CMakeLists.txt file.
# Arguments:
# PLUGIN_NAME The name of the Plugin
function(complex_add_plugin)
  set(options )
  set(oneValueArgs PLUGIN_NAME)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(COMPLEX_PLUGIN_SEARCH_DIRS_2 ${COMPLEX_PLUGIN_SEARCH_DIRS} 
                                      ${complex_SOURCE_DIR}/src/Plugins
                                      ${complex_SOURCE_DIR}/../complex_plugins 
                                      ${complex_SOURCE_DIR}/../DREAM3D_Plugins
                                      )

  #message(STATUS "complex_add_plugin: ${ARGS_PLUGIN_NAME}")
  #message(STATUS "COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR: ${COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR}")
  if(NOT DEFINED COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR OR NOT EXISTS "${COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR}")
    #message(STATUS "COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR was NOT Defined. Searching for Plugins in 'COMPLEX_PLUGIN_SEARCH_DIRS_2'")
    foreach(pluginSearchDir ${COMPLEX_PLUGIN_SEARCH_DIRS_2})
      #message(STATUS "|-Searching:${pluginSearchDir} ")
      if(EXISTS ${pluginSearchDir}/${ARGS_PLUGIN_NAME}/CMakeLists.txt)
        set(COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR ${pluginSearchDir}/${ARGS_PLUGIN_NAME} CACHE PATH "")
        #message(STATUS "  |-Plugin: Defining COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR to ${COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR}")
        break()
      endif()
    endforeach()
  endif()

  # Mark these variables as advanced
  mark_as_advanced(COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR)

  # Now that we have defined where the user's plugin directory is at we
  # need to make sure it has a CMakeLists.txt file in it
  if(EXISTS ${COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR}/CMakeLists.txt)
    set(${ARGS_PLUGIN_NAME}_IMPORT_FILE COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR/CMakeLists.txt)
  endif()

  # By this point we should have everything defined and ready to go...
  if(DEFINED COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR AND DEFINED ${ARGS_PLUGIN_NAME}_IMPORT_FILE)
    #message(STATUS "Plugin: Adding Plugin ${COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR}")

    set(COMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME} ON CACHE BOOL "Enable ${ARGS_PLUGIN_NAME} Plugin")
    if(COMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME})
      #message(STATUS "${PLUGIN_NAME} ENABLED")
      complex_COMPILE_PLUGIN(PLUGIN_NAME ${ARGS_PLUGIN_NAME}
        PLUGIN_SOURCE_DIR ${COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR}
      )
      # Increment the plugin count only if it is enabled
      get_property(PLUGIN_COUNT GLOBAL PROPERTY COMPLEX_PLUGIN_COUNT)
      math(EXPR PLUGIN_COUNT "${PLUGIN_COUNT}+1")
      set_property(GLOBAL PROPERTY COMPLEX_PLUGIN_COUNT ${PLUGIN_COUNT})
    else()
      message(STATUS "${ARGS_PLUGIN_NAME} [DISABLED]: Use '-DCOMPLEX_PLUGIN_ENABLE_${ARGS_PLUGIN_NAME}=ON' to enable this plugin")
    endif()
  else()
    set(COMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR ${pluginSearchDir} CACHE PATH "" FORCE)
    message(STATUS "${ARGS_PLUGIN_NAME} [DISABLED]. Missing Source Directory. Use -DCOMPLEX_${ARGS_PLUGIN_NAME}_SOURCE_DIR=/Path/To/PluginDir")
  endif()
endfunction()

# -----------------------------------------------------------------------------
# This function will search through the COMPLEX_PLUGIN_SEARCH_DIRS variable and
# look for a directory with the name that holds a CMakeLists.txt file.
# Arguments:
# NAME The name of the Plugin
# DESCRIPTION The description of the plugin, used in the "project(...)" call
# VERSION The version of the plugin, used in the "project(...)" call
# FILTER_LIST the list of filters to compile
# ACTION_LIST
function(create_complex_plugin)
  set(options DOC_CHECK ADD_TO_GLOBAL_LIST)
  set(oneValueArgs NAME DESCRIPTION VERSION)
  set(multiValueArgs FILTER_LIST ACTION_LIST ALGORITHM_LIST)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  project(${ARGS_NAME}
    VERSION ${ARGS_VERSION}
    DESCRIPTION ${ARGS_DESCRIPTION}
  )

  set(ARGS_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
  set(ARGS_GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/generated/${ARGS_NAME})
  set(ARGS_EXPORT_HEADER ${ARGS_GENERATED_HEADER_DIR}/${ARGS_NAME}_export.hpp)

  list(LENGTH ARGS_FILTER_LIST PluginNumFilters)
  set_property(GLOBAL PROPERTY ${ARGS_NAME}_filter_count ${PluginNumFilters})

  #------------------------------------------------------------------------------
  # Plugin Headers
  set(${ARGS_NAME}_Plugin_HDRS
    ${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/${ARGS_NAME}Plugin.hpp
  )

  #------------------------------------------------------------------------------
  # Plugin Headers
  set(${ARGS_NAME}_Plugin_SRCS
    ${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/${ARGS_NAME}Plugin.cpp
  )

  #------------------------------------------------------------------------------
  # Plugin Filter Files
  set(${ARGS_NAME}_ARGS_FILTER_FILES)

  set(Filter_Registration_Include_String "")
  set(Public_Filter_Registration_Code "")

  #------------------------------------------------------------------------------
  # Add Filter Sources that need to be compiled.
  # Also generate the Registration code for each of the filters
  foreach(filter ${ARGS_FILTER_LIST})
    list(APPEND ${ARGS_NAME}_Plugin_HDRS
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/${filter}.hpp"
    )
    list(APPEND ${ARGS_NAME}_Plugin_SRCS
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/${filter}.cpp"
    )
    list(APPEND ${ARGS_NAME}_ARGS_FILTER_FILES
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/${filter}.hpp"
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/${filter}.cpp"
    )

    string(APPEND Filter_Registration_Include_String
      "#include \"${ARGS_NAME}/Filters/${filter}.hpp\"\n"
    )

    string(APPEND Public_Filter_Registration_Code
      "  filters.push_back([]() -> complex::IFilter::UniquePointer { return std::make_unique<complex::${filter}>(); });\n"
    )
  endforeach()

  # Include all of the custom Actions
  foreach(action ${ARGS_ACTION_LIST})
    list(APPEND ${ARGS_NAME}_Plugin_HDRS
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/Actions/${action}.hpp"
    )
    list(APPEND ${ARGS_NAME}_Plugin_SRCS
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/Actions/${action}.cpp"
    )
  endforeach()

  # Include all of the algorithms
  foreach(algorithm ${ARGS_ALGORITHM_LIST})
    list(APPEND ${ARGS_NAME}_Plugin_HDRS
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/Algorithms/${algorithm}.hpp"
    )
    list(APPEND ${ARGS_NAME}_Plugin_SRCS
      "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}/Filters/Algorithms/${algorithm}.cpp"
    )
  endforeach()

  configure_file( ${complex_SOURCE_DIR}/cmake/plugin_filter_registration.h.in
    ${ARGS_GENERATED_HEADER_DIR}/${ARGS_NAME}_filter_registration.hpp
  )

  add_library(${ARGS_NAME} SHARED)
  add_library(complex::${ARGS_NAME} ALIAS ${ARGS_NAME})

  set_property(TARGET ${ARGS_NAME} PROPERTY COMPLEX_FILTER_LIST ${ARGS_FILTER_LIST})

  #------------------------------------------------------------------------------
  # Add the plugin to the global list of plugins. This is needed for installation.
  #------------------------------------------------------------------------------
  if(${ARGS_ADD_TO_GLOBAL_LIST})
    get_property(ComplexPluginTargets GLOBAL PROPERTY ComplexPluginTargets)
    set(ComplexPluginTargets ${ComplexPluginTargets} ${ARGS_NAME})
    set_property(GLOBAL PROPERTY ComplexPluginTargets ${ComplexPluginTargets})
  endif()
  #------------------------------------------------------------------------------
  # Add a global property that stores the filters that are in this plugin
  #------------------------------------------------------------------------------
  get_property(COMPLEX_DEBUG_POSTFIX GLOBAL PROPERTY COMPLEX_DEBUG_POSTFIX)   
  set_property(GLOBAL PROPERTY ${ARGS_NAME}_FILTER_LIST ${ARGS_FILTER_LIST})
  get_property(COMPLEX_DEBUG_POSTFIX GLOBAL PROPERTY COMPLEX_DEBUG_POSTFIX)
  set_target_properties(${ARGS_NAME}
    PROPERTIES
      FOLDER "Plugins/${ARGS_NAME}"
      SUFFIX ".complex"
      DEBUG_POSTFIX "${COMPLEX_DEBUG_POSTFIX}"
  )

  #------------------------------------------------------------------------------
  # Sanity check there is a documentation file
  #------------------------------------------------------------------------------
  if(${ARGS_DOC_CHECK})
    foreach(file_name ${ARGS_FILTER_LIST})
      #message(STATUS "CHECKING DOCS FOR ${file_name}")
      #string(REPLACE "Filter" "" file_name ${file_name})
      if(NOT EXISTS "${${ARGS_NAME}_SOURCE_DIR}/docs/${file_name}.rst")
        message(STATUS "!!!> DOCUMENTATION WARNING:${${ARGS_NAME}_SOURCE_DIR}/docs/${file_name}.rst Does Not Exist")
      endif()
    endforeach()
  endif()

  #------------------------------------------------------------------------------
  # Where are the plugins going to be placed during the build, unless overridden
  # by the global property COMPLEX_ARGS_OUTPUT_DIR
  get_property(COMPLEX_ARGS_OUTPUT_DIR GLOBAL PROPERTY COMPLEX_ARGS_OUTPUT_DIR)
  if("${COMPLEX_ARGS_OUTPUT_DIR}" STREQUAL "")
    set_target_properties(${ARGS_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
      RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
    )
  else()
    set_target_properties(${ARGS_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${COMPLEX_ARGS_OUTPUT_DIR}
      RUNTIME_OUTPUT_DIRECTORY ${COMPLEX_ARGS_OUTPUT_DIR}
    )
  endif()

  target_link_libraries(${ARGS_NAME} PUBLIC complex)

  include(${complex_SOURCE_DIR}/cmake/Utility.cmake)
  complex_enable_warnings(TARGET ${ARGS_NAME})

  if(MSVC)
    target_compile_options(${ARGS_NAME}
      PRIVATE
      /MP
    )
  endif()

  generate_export_header(${ARGS_NAME}
    EXPORT_FILE_NAME ${ARGS_EXPORT_HEADER}
  )
  set(${ARGS_NAME}_GENERATED_HEADERS
    ${ARGS_EXPORT_HEADER}
    ${ARGS_GENERATED_HEADER_DIR}/${ARGS_NAME}_filter_registration.hpp
  )
  set(${ARGS_NAME}_ALL_HDRS
    ${${ARGS_NAME}_Plugin_HDRS}
    ${${ARGS_NAME}_GENERATED_HEADERS}
  )

  target_sources(${ARGS_NAME}
    PRIVATE
      ${${ARGS_NAME}_ALL_HDRS}
      ${${ARGS_NAME}_Plugin_SRCS}
  )

  source_group(TREE "${${ARGS_NAME}_SOURCE_DIR}/src/${ARGS_NAME}" PREFIX ${ARGS_NAME} FILES ${${ARGS_NAME}_Plugin_HDRS} ${${ARGS_NAME}_Plugin_SRCS})
  source_group("Generated" FILES ${${ARGS_NAME}_GENERATED_HEADERS})

  target_include_directories(${ARGS_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${${ARGS_NAME}_SOURCE_DIR}/src>
      $<BUILD_INTERFACE:${ARGS_GENERATED_DIR}>
  )

  if(COMPLEX_ENABLE_INSTALL)
    install(TARGETS ${ARGS_NAME}
      PUBLIC_HEADER
        DESTINATION include/${ARGS_NAME}
      RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT ${ARGS_NAME}_Runtime
      LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT ${ARGS_NAME}_Runtime
        NAMELINK_COMPONENT ${ARGS_NAME}_Development
      ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT ${ARGS_NAME}_Development
    )
  endif()
endfunction()

# -----------------------------------------------------------------------------
# This function will create a unit test for the given PLUGIN_NAME that uses the
# Catch2 unit testing framework
# Arguments:
# PLUGIN_NAME The name of the Plugin
# FILTER_LIST The list of sources to compile.
function(create_complex_plugin_unit_test)
  set(options)
  set(oneValueArgs PLUGIN_NAME)
  set(multiValueArgs FILTER_LIST)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(UNIT_TEST_TARGET ${ARGS_PLUGIN_NAME}UnitTest)

  #------------------------------------------------------------------------------
  # Find the Catch2 unit testing package
  find_package(Catch2 CONFIG REQUIRED)
  include(Catch)

  get_filename_component(complexProj_PARENT ${complex_SOURCE_DIR} DIRECTORY CACHE)
  if("${DREAM3D_DATA_DIR}" STREQUAL "")
    set(DREAM3D_DATA_DIR "${complexProj_PARENT}/DREAM3D_Data/" CACHE PATH "The directory where to find test data files")
  endif()

  if(NOT EXISTS "${DREAM3D_DATA_DIR}")
    file(MAKE_DIRECTORY "${DREAM3D_DATA_DIR}")
    message(STATUS "DREAM3D_Data does not exist at path '${DREAM3D_DATA_DIR}'. It will be created.")
    #set(DREAM3D_DATA_DIR "DREAM3D_DATA_DIR DOES NOT EXIST")
  endif()

  file(TO_CMAKE_PATH "${DREAM3D_DATA_DIR}" DREAM3D_DATA_DIR_NORM)

  #------------------------------------------------------------------------------
  # Convert the native path to a path that will be compatible with C++ source codes
  file(TO_CMAKE_PATH "${${ARGS_PLUGIN_NAME}_SOURCE_DIR}" PLUGIN_SOURCE_DIR_NORM)
  file(TO_CMAKE_PATH "${complex_SOURCE_DIR}" COMPLEX_SOURCE_DIR_NORM)

  #------------------------------------------------------------------------------
  # Set the generated directory in the build folder, set the path to the generated
  # header, and finally configure the header file
  set(${ARGS_PLUGIN_NAME}_GENERATED_DIR ${complex_BINARY_DIR}/Plugins/${ARGS_PLUGIN_NAME}/generated)
  set(COMPLEX_TEST_DIRS_HEADER ${${ARGS_PLUGIN_NAME}_GENERATED_DIR}/${ARGS_PLUGIN_NAME}/${ARGS_PLUGIN_NAME}_test_dirs.hpp)
  configure_file(${${ARGS_PLUGIN_NAME}_SOURCE_DIR}/test/test_dirs.hpp.in ${COMPLEX_TEST_DIRS_HEADER} @ONLY)
  file(MAKE_DIRECTORY "${${ARGS_PLUGIN_NAME}_BINARY_DIR}/test_output/")

  #------------------------------------------------------------------------------
  # Create the unit test executable
  add_executable(${UNIT_TEST_TARGET}
    ${COMPLEX_TEST_DIRS_HEADER}
    ${ARGS_PLUGIN_NAME}_test_main.cpp
    ${${ARGS_PLUGIN_NAME}UnitTest_SRCS}
  )

  target_link_libraries(${UNIT_TEST_TARGET}
    PRIVATE
      complex
      ${ARGS_PLUGIN_NAME}
      Catch2::Catch2
      complex::UnitTestCommon
  )

  source_group("test" FILES  ${${ARGS_PLUGIN_NAME}UnitTest_SRCS} ${ARGS_PLUGIN_NAME}_test_main.cpp)
  source_group("Generated" FILES ${COMPLEX_TEST_DIRS_HEADER} )                                                

  include(${complex_SOURCE_DIR}/cmake/Utility.cmake)
  complex_enable_warnings(TARGET ${UNIT_TEST_TARGET})

  #------------------------------------------------------------------------------
  # Require that the test plugins are built before tests because some tests
  # require loading from those plugins but don't want to link to them.
  add_dependencies(${UNIT_TEST_TARGET} ${ARGS_PLUGIN_NAME})

  set_target_properties(${UNIT_TEST_TARGET}
    PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
      FOLDER "Plugins/${ARGS_PLUGIN_NAME}"
  )

  target_compile_definitions(${UNIT_TEST_TARGET}
    PRIVATE
      COMPLEX_BUILD_DIR="$<TARGET_FILE_DIR:complex_test>"
  )

  target_compile_options(${UNIT_TEST_TARGET}
    PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/MP>
  )

  target_include_directories(${UNIT_TEST_TARGET}
    PRIVATE
      ${${ARGS_PLUGIN_NAME}_GENERATED_DIR}
  )

  catch_discover_tests(${UNIT_TEST_TARGET})
endfunction()
