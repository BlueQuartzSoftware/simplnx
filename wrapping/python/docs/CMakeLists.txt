#------------------------------------------------------------------------------
# Compile the executable that will generate the .rst files for each filter
#------------------------------------------------------------------------------
add_executable(generate_sphinx_docs)

set_target_properties(generate_sphinx_docs
  PROPERTIES
    DEBUG_POSTFIX "${SIMPLNX_DEBUG_POSTFIX}"
)

target_sources(generate_sphinx_docs
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_sphinx_docs.cpp
)
target_link_libraries(generate_sphinx_docs PRIVATE simplnx::simplnx)
target_compile_definitions(generate_sphinx_docs
  PRIVATE
    SIMPLNX_BIN_DIR="$<TARGET_FILE_DIR:generate_sphinx_docs>"
    SIMPLNX_BUILD_DIR="${simplnx_BINARY_DIR}"
    SIMPLNX_PLUGIN_COUNT=${SIMPLNX_PLUGIN_COUNT}
    SIMPLNX_SOURCE_DIR="${simplnx_SOURCE_DIR}"
)

target_include_directories(generate_sphinx_docs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp" "#pragma once\n#include <map>\n#include <string>\nstatic std::map<std::string, std::string> s_PluginDirMap =\n{\n")

#------------------------------------------------------------------------------
# Add dependencies to generate_sphinx_docs to ensure it will not run until
# after all targets are compiled.
#------------------------------------------------------------------------------
get_property(simplnxPluginTargets GLOBAL PROPERTY simplnxPluginTargets)
foreach(d3dPlugin ${simplnxPluginTargets})
  add_dependencies(generate_sphinx_docs ${d3dPlugin})
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp" "{\"${d3dPlugin}\", \"${${d3dPlugin}_SOURCE_DIR}\"},\n")
endforeach()
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp" "};\n")

target_sources(generate_sphinx_docs
  PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp"
)
set_target_properties(generate_sphinx_docs PROPERTIES FOLDER simplnx_Python__Documentation)

set(SIMPLNX_SPHINX_DOCS_DIR "${simplnx_BINARY_DIR}/wrapping/python/docs")

#------------------------------------------------------------------------------
# Remove the destination directory entirely first, this removes stale files
# Copy the entire doc source dir down into the build directory
#------------------------------------------------------------------------------
add_custom_target(sphinx_copy_sources ALL
  DEPENDS generate_sphinx_docs
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${SIMPLNX_SPHINX_DOCS_DIR}/source"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/source" "${SIMPLNX_SPHINX_DOCS_DIR}/source"
  COMMENT "SIMPLNX Python Docs: Copying static doc files to build directory at ${SIMPLNX_SPHINX_DOCS_DIR}"
)
set_target_properties(sphinx_copy_sources PROPERTIES FOLDER simplnx_Python__Documentation)

#------------------------------------------------------------------------------
# Execute the sphinx rst plugin generation tool
#------------------------------------------------------------------------------
add_custom_target(sphinx_generate_plugin_rst ALL
  DEPENDS sphinx_copy_sources
  COMMAND "$<TARGET_FILE:generate_sphinx_docs>"
  COMMENT "SIMPLNX Python Docs: Generated Plugin .rst docs file"
)
set_target_properties(sphinx_generate_plugin_rst PROPERTIES FOLDER simplnx_Python__Documentation)

#------------------------------------------------------------------------------
# Find the `sphinx-build` executable
#------------------------------------------------------------------------------
find_package(Sphinx REQUIRED)

#------------------------------------------------------------------------------
# Ensure this directory is created
#------------------------------------------------------------------------------
file(MAKE_DIRECTORY "${SIMPLNX_SPHINX_DOCS_DIR}")

#------------------------------------------------------------------------------
# Configure the make files for either windows or unix
#------------------------------------------------------------------------------
if(WIN32)
  set(SPHINX_MAKE_BAT_FILE "${SIMPLNX_SPHINX_DOCS_DIR}/make.bat")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/make.bat.in" ${SPHINX_MAKE_BAT_FILE})
else()
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in" "${SIMPLNX_SPHINX_DOCS_DIR}/Makefile")
endif()

#------------------------------------------------------------------------------
# Add the custom target to run sphinx-build and create the documentation
#------------------------------------------------------------------------------
if(WIN32)
  add_custom_target(sphinx_docs_generation ALL
    DEPENDS sphinx_generate_plugin_rst
    COMMAND "${SPHINX_MAKE_BAT_FILE}" clean
    COMMAND "${SPHINX_MAKE_BAT_FILE}" html
    COMMENT "SIMPLNX Python Docs: Generating Python HTML Documentation"
    WORKING_DIRECTORY "${SIMPLNX_SPHINX_DOCS_DIR}"
  )
else()
  add_custom_target(sphinx_docs_generation ALL
    DEPENDS sphinx_generate_plugin_rst
    COMMAND "/usr/bin/make" clean
    COMMAND "/usr/bin/make" html
    COMMENT "SIMPLNX Python Docs: Generating Python HTML Documentation"
    WORKING_DIRECTORY "${SIMPLNX_SPHINX_DOCS_DIR}"
  )
endif()
set_target_properties(sphinx_docs_generation PROPERTIES FOLDER simplnx_Python__Documentation)
