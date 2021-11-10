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
  set(oneValueArgs PLUGIN_NAME PLUGIN_SOURCE_DIR)
  cmake_parse_arguments(P "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  option(COMPLEX_PLUGIN_ENABLE_${P_PLUGIN_NAME} "Build the ${P_PLUGIN_NAME}" ON)

  if(COMPLEX_PLUGIN_ENABLE_${P_PLUGIN_NAME})
    add_subdirectory(${P_PLUGIN_SOURCE_DIR} ${PROJECT_BINARY_DIR}/Plugins/${P_PLUGIN_NAME})
    get_property(PluginNumFilters GLOBAL PROPERTY ${P_PLUGIN_NAME}_filter_count)

    message(STATUS "${P_PLUGIN_NAME} [ENABLED] ${PluginNumFilters} Filters")
    #- Now set up the dependency between the main application and each of the plugins so that
    #- things like Visual Studio are forced to rebuild the plugins when launching
    #- the sandbox application
    if(COMPLEX_PLUGIN_ENABLE_${P_PLUGIN_NAME} AND TARGET complex::complex AND TARGET ${P_PLUGIN_NAME})
      add_dependencies(${P_PLUGIN_NAME} complex::complex )
    endif()

  else()
    message(STATUS "${P_PLUGIN_NAME} [DISABLED]: Use -DCOMPLEX_PLUGIN_ENABLE_${P_PLUGIN_NAME}=ON to Enable Plugin")
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
  #message(STATUS "COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR: ${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR}")
  if(NOT DEFINED COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR OR NOT EXISTS "${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR}")
    #message(STATUS "COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR was NOT Defined. Searching for Plugins in 'COMPLEX_PLUGIN_SEARCH_DIRS'")
    foreach(pluginSearchDir ${COMPLEX_PLUGIN_SEARCH_DIRS})
      #message(STATUS "|-Searching:${pluginSearchDir} ")
      if(EXISTS ${pluginSearchDir}/${P_PLUGIN_NAME}/CMakeLists.txt)
        set(COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR ${pluginSearchDir}/${P_PLUGIN_NAME} CACHE PATH "")
        #message(STATUS "  |-Plugin: Defining COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR to ${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR}")
        break()
      endif()
    endforeach()

    #message(STATUS "${complex_SOURCE_DIR}/../complex_plugins/${P_PLUGIN_NAME}/CMakeLists.txt")
    if(EXISTS ${complex_SOURCE_DIR}/../complex_plugins/${P_PLUGIN_NAME}/CMakeLists.txt)
      set(COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR ${complex_SOURCE_DIR}/../complex_plugins/${P_PLUGIN_NAME} CACHE PATH "")
      #message(STATUS "  |-Plugin: Defining COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR to ${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR}")
    endif()

  endif()

  # Mark these variables as advanced
  mark_as_advanced(COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR)

  # Now that we have defined where the user's plugin directory is at we
  # need to make sure it has a CMakeLists.txt file in it
  if(EXISTS ${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR}/CMakeLists.txt)
    set(${P_PLUGIN_NAME}_IMPORT_FILE COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR/CMakeLists.txt)
  endif()

  # By this point we should have everything defined and ready to go...
  if(DEFINED COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR AND DEFINED ${P_PLUGIN_NAME}_IMPORT_FILE)
      #message(STATUS "Plugin: Adding Plugin ${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR}")
      complex_COMPILE_PLUGIN(PLUGIN_NAME ${P_PLUGIN_NAME}
                             PLUGIN_SOURCE_DIR ${COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR})
  else()
      set(COMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR ${pluginSearchDir} CACHE PATH "" FORCE)
      message(STATUS "${P_PLUGIN_NAME} [DISABLED]. Missing Source Directory. Use -DCOMPLEX_${P_PLUGIN_NAME}_SOURCE_DIR=/Path/To/PluginDir")
  endif()

endfunction()

# -----------------------------------------------------------------------------
# This function will search through the COMPLEX_PLUGIN_SEARCH_DIRS variable and
# look for a directory with the name that holds a CMakeLists.txt file.
# Arguments:
# PLUGIN_NAME The name of the Plugin
# FILTER_LIST the list of filters to compile
# DESCRIPTION The description of the plugin, used in the "PROJECT(...)" call
# VERSION The version of the plugin, used in the "PROJECT(...)" call
# 
function(create_complex_plugin)
  set(options)
  set(oneValueArgs NAME DESCRIPTION VERSION)
  set(multiValueArgs FILTER_LIST)
  cmake_parse_arguments(PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  
  PROJECT(${PLUGIN_NAME}
      VERSION ${PLUGIN_VERSION}
      DESCRIPTION ${PLUGIN_DESCRIPTION}
  )

  set(PLUGIN_GENERATED_DIR ${PROJECT_BINARY_DIR}/generated)
  set(PLUGIN_GENERATED_HEADER_DIR ${PROJECT_BINARY_DIR}/generated/${PLUGIN_NAME})
  set(PLUGIN_EXPORT_HEADER ${PLUGIN_GENERATED_HEADER_DIR}/${PLUGIN_NAME}_export.hpp)


  list(LENGTH PLUGIN_FILTER_LIST PluginNumFilters)
  set_property(GLOBAL PROPERTY ${PLUGIN_NAME}_filter_count ${PluginNumFilters})

  #------------------------------------------------------------------------------
  # Plugin Headers
  set(${PLUGIN_NAME}_Plugin_HDRS
      ${${PLUGIN_NAME}_SOURCE_DIR}/${PLUGIN_NAME}Plugin.hpp
  )

  #------------------------------------------------------------------------------
  # Plugin Headers
  set(${PLUGIN_NAME}_Plugin_SRCS
      ${${PLUGIN_NAME}_SOURCE_DIR}/${PLUGIN_NAME}Plugin.cpp
  )

  #------------------------------------------------------------------------------
  # Plugin Filter Files
  set(${PLUGIN_NAME}_PLUGIN_FILTER_FILES)

  set(Filter_Registration_Include_String "")
  set(Filter_Registration_Code "")

  #------------------------------------------------------------------------------
  # Add Filter Sources that need to be compiled.
  # Also generate the Registration code for each of the filters 
  foreach(filter ${PLUGIN_FILTER_LIST})
    list(APPEND ${PLUGIN_NAME}_Plugin_HDRS
      "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/${filter}.hpp"
    )
    list(APPEND ${PLUGIN_NAME}_Plugin_SRCS
      "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/${filter}.cpp"
    )
    list(APPEND ${PLUGIN_NAME}_PLUGIN_FILTER_FILES
      "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/${filter}.hpp"
      "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}/Filters/${filter}.cpp"
    )

    string(APPEND Filter_Registration_Include_String
      "#include \"${PLUGIN_NAME}/Filters/${filter}.hpp\"\n")

    string(APPEND Filter_Registration_Code
      "  addFilter([]() -> IFilter::UniquePointer { return std::make_unique<${filter}>(); });\n")

  endforeach()

  configure_file( ${complex_SOURCE_DIR}/cmake/plugin_filter_registration.h.in
                  ${PLUGIN_GENERATED_HEADER_DIR}/plugin_filter_registration.h)

  add_library(${PLUGIN_NAME} SHARED)
  add_library(complex::${PLUGIN_NAME} ALIAS ${PLUGIN_NAME})

  set_target_properties(${PLUGIN_NAME} PROPERTIES
    FOLDER "Plugins/${PLUGIN_NAME}"
    SUFFIX ".complex")

  #------------------------------------------------------------------------------
  # Where are the plugins going to be placed during the build, unless overridden
  # by the global property COMPLEX_PLUGIN_OUTPUT_DIR
  get_property(COMPLEX_PLUGIN_OUTPUT_DIR GLOBAL PROPERTY COMPLEX_PLUGIN_OUTPUT_DIR)
  if("${COMPLEX_PLUGIN_OUTPUT_DIR}" STREQUAL "")
    set_target_properties(${PLUGIN_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
      RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
    )
  else()
    set_target_properties(${PLUGIN_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY ${COMPLEX_PLUGIN_OUTPUT_DIR}
      RUNTIME_OUTPUT_DIRECTORY ${COMPLEX_PLUGIN_OUTPUT_DIR}
    )
  endif()

  target_link_libraries(${PLUGIN_NAME} PUBLIC complex)

  if(MSVC)
    target_compile_options(${PLUGIN_NAME}
      PRIVATE
        /MP
    )
  endif()

  generate_export_header(${PLUGIN_NAME}
    EXPORT_FILE_NAME ${PLUGIN_EXPORT_HEADER}
  )
  set(${PLUGIN_NAME}_GENERATED_HEADERS
    ${PLUGIN_EXPORT_HEADER}
  )
  set(${PLUGIN_NAME}_ALL_HDRS
    ${${PLUGIN_NAME}_Plugin_HDRS}
    ${${PLUGIN_NAME}_GENERATED_HEADERS}
  )

  target_sources(${PLUGIN_NAME}
    PRIVATE
      ${${PLUGIN_NAME}_ALL_HDRS}
      ${${PLUGIN_NAME}_Plugin_SRCS}
  )

  source_group(TREE "${${PLUGIN_NAME}_SOURCE_DIR}/src/${PLUGIN_NAME}" PREFIX ${PLUGIN_NAME} FILES ${${PLUGIN_NAME}_PLUGIN_FILTER_FILES})

  target_include_directories(${PLUGIN_NAME}
      PUBLIC
      $<BUILD_INTERFACE:${${PLUGIN_NAME}_SOURCE_DIR}/src>
      $<BUILD_INTERFACE:${PLUGIN_GENERATED_DIR}>
      $<BUILD_INTERFACE:${COMPLEX_BINARY_DIR}/Plugins/${PLUGIN_NAME}>
  )

  install(TARGETS ${PLUGIN_NAME}
      PUBLIC_HEADER DESTINATION include/${PLUGIN_NAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          COMPONENT   ${PLUGIN_NAME}_Runtime
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          COMPONENT   ${PLUGIN_NAME}_Runtime
          NAMELINK_COMPONENT ${PLUGIN_NAME}_Development
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
          COMPONENT   ${PLUGIN_NAME}_Development
  )

endfunction()

# -----------------------------------------------------------------------------
# This function will create a unit test for the given PLUGIN_NAME that uses the
# Catch2 unit testing framework
# Arguments:
# PLUGIN_NAME The name of the Plugin
# FILTER_LIST The list of sources to compile.
# 
function(create_complex_plugin_unit_test)
  set(options)
  set(oneValueArgs PLUGIN_NAME)
  set(multiValueArgs FILTER_LIST)
  cmake_parse_arguments(UT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  #------------------------------------------------------------------------------
  # Find the Catch2 unit testing package
  find_package(Catch2 CONFIG REQUIRED)
  include(Catch)

  #------------------------------------------------------------------------------
  # Convert the native path to a path that will be compatible with C++ source codes
  file(TO_CMAKE_PATH "${${UT_PLUGIN_NAME}_SOURCE_DIR}" PLUGIN_SOURCE_DIR_NORM)

  #------------------------------------------------------------------------------
  # Set the generated directory in the build folder, set the path to the generated
  # header, and finally configure the header file
  set(${UT_PLUGIN_NAME}_GENERATED_DIR ${complex_BINARY_DIR}/Plugins/${UT_PLUGIN_NAME}/generated)
  set(COMPLEX_TEST_DIRS_HEADER ${${UT_PLUGIN_NAME}_GENERATED_DIR}/${UT_PLUGIN_NAME}/${UT_PLUGIN_NAME}_test_dirs.hpp)
  configure_file(${${UT_PLUGIN_NAME}_SOURCE_DIR}/test/test_dirs.hpp.in ${COMPLEX_TEST_DIRS_HEADER} @ONLY)

  #------------------------------------------------------------------------------
  # Create the unit test executable
  add_executable(${UT_PLUGIN_NAME}UnitTest
            ${COMPLEX_TEST_DIRS_HEADER}
            ${UT_PLUGIN_NAME}_test_main.cpp
            ${${UT_PLUGIN_NAME}UnitTest_SRCS}
  )

  target_link_libraries(${UT_PLUGIN_NAME}UnitTest
    PRIVATE complex ${UT_PLUGIN_NAME} Catch2::Catch2
  )

  #------------------------------------------------------------------------------
  # Require that the test plugins are built before tests because some tests
  # require loading from those plugins but don't want to link to them.
  add_dependencies(${UT_PLUGIN_NAME}UnitTest ${UT_PLUGIN_NAME})

  set_target_properties(${UT_PLUGIN_NAME}UnitTest
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
  )

  target_compile_definitions(${UT_PLUGIN_NAME}UnitTest
    PRIVATE
    COMPLEX_BUILD_DIR="$<TARGET_FILE_DIR:complex_test>"
  )

  target_compile_options(${UT_PLUGIN_NAME}UnitTest
    PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
  )

  target_include_directories(${UT_PLUGIN_NAME}UnitTest PRIVATE ${${UT_PLUGIN_NAME}_GENERATED_DIR})

  catch_discover_tests(${UT_PLUGIN_NAME}UnitTest)

endfunction()
