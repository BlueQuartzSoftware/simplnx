#------------------------------------------------------------------------------
# Compile the executable that will generate the .rst files for each filter
#------------------------------------------------------------------------------
add_executable(generate_sphinx_docs)

set_target_properties(generate_sphinx_docs
  PROPERTIES
    DEBUG_POSTFIX "${COMPLEX_DEBUG_POSTFIX}"
)

target_sources(generate_sphinx_docs
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_sphinx_docs.cpp
)
target_link_libraries(generate_sphinx_docs PRIVATE complex::complex)
target_compile_definitions(generate_sphinx_docs
                           PRIVATE
                           COMPLEX_BIN_DIR="$<TARGET_FILE_DIR:generate_sphinx_docs>"
                           COMPLEX_BUILD_DIR="${complex_BINARY_DIR}"
                           COMPLEX_PLUGIN_COUNT=${COMPLEX_PLUGIN_COUNT}
                           COMPLEX_SOURCE_DIR="${complex_SOURCE_DIR}"
)


target_include_directories(generate_sphinx_docs PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp" "#pragma once\n#include <map>\n#include <string>\nstatic std::map<std::string, std::string> s_PluginDirMap =\n{\n")

#------------------------------------------------------------------------------
# Add dependencies to generate_sphinx_docs to ensure it will not run until
# after all targets are compiled.
#------------------------------------------------------------------------------
get_property(ComplexPluginTargets GLOBAL PROPERTY ComplexPluginTargets)
foreach(d3dPlugin ${ComplexPluginTargets})
  add_dependencies(generate_sphinx_docs ${d3dPlugin})
  file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp" "{\"${d3dPlugin}\", \"${${d3dPlugin}_SOURCE_DIR}\"},\n")
endforeach()
file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp" "};\n")

target_sources(generate_sphinx_docs
  PRIVATE
  "${CMAKE_CURRENT_BINARY_DIR}/plugin_source_dirs.hpp"
)

#------------------------------------------------------------------------------
# Remove the destination directory entirely first, this removes stale files
# Copy the entire doc source dir down into the build directory
#------------------------------------------------------------------------------
add_custom_target(sphinx_copy_sources ALL
  DEPENDS generate_sphinx_docs
  COMMAND ${CMAKE_COMMAND} -E remove_directory "${complex_BINARY_DIR}/wrapping/python/docs/source"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/source" "${complex_BINARY_DIR}/wrapping/python/docs/source"
  COMMENT "SPHINX: Copying static doc files to build directory at ${complex_BINARY_DIR}/wrapping/python/docs/"
)


#------------------------------------------------------------------------------
# Execute the sphinx rst plugin generation tool
#------------------------------------------------------------------------------
add_custom_target(sphinx_generate_plugin_rst ALL
  DEPENDS sphinx_copy_sources
  COMMAND "$<TARGET_FILE:generate_sphinx_docs>"
  COMMENT "SPHINX: Generated Plugin .rst docs file"
)

#------------------------------------------------------------------------------
# Find the `sphinx-build` executable
#------------------------------------------------------------------------------
get_filename_component(PYTHON_EXE_PATH "${Python3_EXECUTABLE}" DIRECTORY CACHE)
set(SPHINX_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
find_program(SPHINX_EXECUTABLE sphinx-build
    NAMES sphinx-buildd sphinx-build.exe
    PATHS "${PYTHON_EXE_PATH};${PYTHON_EXE_PATH}/Scripts"
)

if(NOT SPHINX_EXECUTABLE)
  if("${SPHINX_EXECUTABLE}" STREQUAL "SPHINX_EXECUTABLE-NOTFOUND")
    message(FATAL_ERROR "sphinx-build was not found on your system. Typical installation under python is\n\
    `pip install sphinx` or `conda install sphinx myst-parser sphinx-markdown-tables sphinx_rtd_theme`\n\
    You can also define Python3_EXECUTABLE and SPHINX_EXECUTABLE on the command line\n\
    using the standard '-DPython3_EXECUTABLE=.....' syntax")

    message(STATUS "Python3_EXECUTABLE=${Python3_EXECUTABLE}")
    message(STATUS "PYTHON_EXE_PATH=${PYTHON_EXE_PATH}")
    message(STATUS "SPHINX_PYTHON_EXECUTABLE=${SPHINX_PYTHON_EXECUTABLE}")
    message(STATUS "SPHINX_EXECUTABLE=${SPHINX_EXECUTABLE}")

  endif()
endif()

#------------------------------------------------------------------------------
# Ensure this directory is created
#------------------------------------------------------------------------------
FILE(MAKE_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs")

#------------------------------------------------------------------------------
# Configure the make files for either windows or unix
#------------------------------------------------------------------------------
if(WIN32)
  configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/make.bat.in" "${complex_BINARY_DIR}/wrapping/python/docs/make.bat")
else()
  configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in" "${complex_BINARY_DIR}/wrapping/python/docs/Makefile")
endif()

#------------------------------------------------------------------------------
# Add the custom target to run sphinx-build and create the documentation
#------------------------------------------------------------------------------
FILE(MAKE_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs")
if(WIN32)
  add_custom_target(sphinx_docs_generation ALL
    DEPENDS sphinx_generate_plugin_rst
    COMMAND "${complex_BINARY_DIR}/wrapping/python/docs/make.bat"
    COMMENT "SPHINX: Generating Python HTML Documentation"
    WORKING_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs"
  )
else()
  add_custom_target(sphinx_docs_generation ALL
    DEPENDS sphinx_generate_plugin_rst
    COMMAND "/usr/bin/make" clean
    COMMAND "/usr/bin/make" html
    COMMENT "SPHINX: Generating Python HTML Documentation"
    WORKING_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs"
  )
endif()
