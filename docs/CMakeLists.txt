option(SIMPLNX_BUILD_SPHINX_DOCS "Use Sphinx to builddocumentation" OFF)

find_package(Doxygen REQUIRED)

set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src/simplnx)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs/doxygen)
set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/html/index.html)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

set(DOXYGEN_STRIP_PATH ${PROJECT_SOURCE_DIR}/src)
if(SIMPLNX_BUILD_SPHINX_DOCS)
  set(DOXYGEN_GENERATE_XML YES)
else()
  set(DOXYGEN_GENERATE_XML NO)
endif()

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
  DEPENDS ${SIMPLNX_HDRS}
  COMMAND Doxygen::doxygen ${DOXYFILE_OUT}
  MAIN_DEPENDENCY ${DOXYFILE_IN} ${DOXYFILE_OUT}
  COMMENT "Generating ${PROJECT_NAME} docs with Doxygen"
  VERBATIM
)

option(SIMPLNX_DOXYGEN_EXCLUDE_FROM_ALL "Exclude the Doxygen generation from the default target" OFF)
mark_as_advanced(SIMPLNX_DOXYGEN_EXCLUDE_FROM_ALL)

if(SIMPLNX_SPHINX_EXCLUDE_FROM_ALL)
  set(doxygen_default)
else()
  set(doxygen_default ALL)
endif()

add_custom_target(simplnx_doxygen ${doxygen_default} DEPENDS ${DOXYGEN_INDEX_FILE})

option(SIMPLNX_INSTALL_DOXYGEN_DOCS "Enable install forDoxygen docs" OFF)

include(GNUInstallDirs)

if(SIMPLNX_INSTALL_DOXYGEN_DOCS)
  install(DIRECTORY ${DOXYGEN_OUTPUT_DIR}
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
  )
endif()

if(SIMPLNX_BUILD_SPHINX_DOCS)
  find_package(Sphinx REQUIRED)

  set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
  set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/docs/sphinx)
  set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

  add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND ${SPHINX_BUILD_EXECUTABLE}
      -b html
      ${SPHINX_SOURCE}
      ${SPHINX_BUILD}
      -Dbreathe_projects.${PROJECT_NAME}=${DOXYGEN_OUTPUT_DIR}/xml
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
      ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
      ${DOXYGEN_INDEX_FILE}
    MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
    COMMENT "Generating ${PROJECT_NAME} docs with Sphinx"
    VERBATIM
  )

  option(SIMPLNX_SPHINX_EXCLUDE_FROM_ALL "Exclude the Sphinx generation from the default target" OFF)
  mark_as_advanced(SIMPLNX_SPHINX_EXCLUDE_FROM_ALL)

  if(SIMPLNX_SPHINX_EXCLUDE_FROM_ALL)
    set(sphinx_default)
  else()
    set(sphinx_default ALL)
  endif()

  add_custom_target(simplnx_sphinx ${sphinx_default} DEPENDS ${SPHINX_INDEX_FILE})

  option(SIMPLNX_INSTALL_SPHINX_DOCS "Enable install forSphinx docs" OFF)
  if(SIMPLNX_INSTALL_SPHINX_DOCS)
    install(DIRECTORY ${SPHINX_BUILD}
      DESTINATION ${CMAKE_INSTALL_DOCDIR}
    )
  endif()
endif()
