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

#------------------------------------------------------------------------------
# Execute the sphinx rst plugin generation tool
#------------------------------------------------------------------------------
add_custom_target(sphinx_generate_plugin_rst ALL
  DEPENDS generate_sphinx_docs
  COMMAND "$<TARGET_FILE:generate_sphinx_docs>"
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

if(WIN32)
  configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/make.bat.in" "${complex_BINARY_DIR}/wrapping/python/docs/make.bat")
else()
  configure_file( "${CMAKE_CURRENT_SOURCE_DIR}/Makefile.in" "${complex_BINARY_DIR}/wrapping/python/docs/Makefile")
endif()

FILE(MAKE_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs")

if(WIN32)
  add_custom_target(sphinx_docs_generation ALL
    DEPENDS sphinx_generate_plugin_rst
    COMMAND "${complex_BINARY_DIR}/wrapping/python/docs/make.bat"
    COMMENT "Sphinx: Generating Python HTML Documentation"
    WORKING_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs"
  )
else()
  add_custom_target(sphinx_docs_generation ALL
    DEPENDS sphinx_generate_plugin_rst
    COMMAND "/usr/bin/make" clean
    COMMAND "/usr/bin/make" html
    COMMENT "Sphinx: Generating Python HTML Documentation"
    WORKING_DIRECTORY "${complex_BINARY_DIR}/wrapping/python/docs"
  )
endif()
