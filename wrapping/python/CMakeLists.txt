find_package(pybind11 2.10.0 CONFIG REQUIRED)

add_library(CxPybind INTERFACE)

target_include_directories(CxPybind INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/CxPybind)

target_link_libraries(CxPybind
  INTERFACE
    complex
    pybind11::headers
)

pybind11_add_module(complexpy complexpy.cpp)

# we want the CMake target name to be "complexpy", but the Python module to be "complex"
set_target_properties(complexpy
  PROPERTIES
    OUTPUT_NAME ${COMPLEX_PYTHON_MODULE}
    ARCHIVE_OUTPUT_NAME ${COMPLEX_PYTHON_MODULE}py
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)

# pybind11 sets CXX_VISIBILITY_PRESET to hidden (though this may change in the future see https://github.com/pybind/pybind11/pull/4072)
# This can cause issues when using libc++ and std::any across shared libraries
# The hidden visibility causes some objects to have different RTTI which then results in std::bad_any_cast
option(COMPLEX_PY_DISABLE_HIDDEN_VISIBILITY "Controls CXX_VISIBILITY_PRESET for python modules" OFF)

if(COMPLEX_PY_DISABLE_HIDDEN_VISIBILITY)
  set_target_properties(complexpy
    PROPERTIES
      CXX_VISIBILITY_PRESET default
  )
endif()

target_link_libraries(complexpy
  PUBLIC
    ComplexCore
    CxPybind
)

target_compile_options(complexpy
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

function(complex_create_pyi)
  set(options)
  set(oneValueArgs PYTHON_MODULE_NAME PYTHON_TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  find_program(MYPY_STUBGEN_EXE stubgen REQUIRED)

  set(PYI_TARGET_NAME ${ARGS_PYTHON_MODULE_NAME}CreateStubFile)

  add_custom_target(${PYI_TARGET_NAME} ALL
    COMMAND ${MYPY_STUBGEN_EXE} -m ${ARGS_PYTHON_MODULE_NAME} -o $<TARGET_FILE_DIR:complex>
    COMMENT "${ARGS_PYTHON_MODULE_NAME}: Generating .pyi files"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:complex>
  )
  set_property(TARGET ${PYI_TARGET_NAME} PROPERTY FOLDER "Python/Stub")
  add_dependencies(${PYI_TARGET_NAME} ${ARGS_PYTHON_TARGET})
endfunction()

function(complex_create_plugin_pyi)
  set(options)
  set(oneValueArgs PLUGIN_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(TOLOWER ${ARGS_PLUGIN_NAME} PLUGIN_NAME_LOWER)

  complex_create_pyi(PYTHON_MODULE_NAME ${PLUGIN_NAME_LOWER} PYTHON_TARGET ${PLUGIN_NAME_LOWER})
  add_dependencies(${PLUGIN_NAME_LOWER} complexpy)
endfunction()

option(COMPLEX_PY_GENERATE_PYI "Generate .pyi files for python bindings" OFF)
if(COMPLEX_PY_GENERATE_PYI)
  #complex_create_pyi(PYTHON_MODULE_NAME complex PYTHON_TARGET complexpy)
endif()

option(COMPLEX_PY_ENABLE_DOCS "Generate python documentation using sphinx" OFF)
if(COMPLEX_PY_ENABLE_DOCS)
  find_package(Sphinx REQUIRED)

  set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
  set(SPHINX_BUILD ${PROJECT_BINARY_DIR}/docs/python)

  add_custom_target(complexpy_docs
    COMMAND
      ${CMAKE_COMMAND} -E env "PYTHONPATH=$<TARGET_FILE_DIR:complex>"
      ${SPHINX_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
    WORKING_DIRECTORY $<TARGET_FILE_DIR:complex>
    COMMENT "Generating complexpy docs with Sphinx"
    VERBATIM
  )
  add_dependencies(complexpy_docs complexCreateStubFile)
endif()

option(COMPLEX_PY_ENABLE_INSTALL "Enables install complexpy" OFF)
if(COMPLEX_PY_ENABLE_INSTALL)
  include(GNUInstallDirs)
  set(COMPLEX_PY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Path to install complexpy")

  install(TARGETS complexpy
    LIBRARY
      DESTINATION ${COMPLEX_PY_INSTALL_DIR}
  )
  if(COMPLEX_PY_GENERATE_PYI)
    # install(FILES $<TARGET_FILE_DIR:complex>/complex.pyi
    #   DESTINATION ${COMPLEX_PY_INSTALL_DIR}
    # )
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/complex.pyi
      DESTINATION ${COMPLEX_PY_INSTALL_DIR}
    )
  endif()
endif()

function(complex_add_python_plugin)
  set(options)
  set(oneValueArgs PLUGIN_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(TOLOWER ${ARGS_PLUGIN_NAME} PYTHON_MODULE_NAME)

  pybind11_add_module(${PYTHON_MODULE_NAME} ${PYTHON_MODULE_NAME}.cpp)

  target_link_libraries(${PYTHON_MODULE_NAME}
    PUBLIC
      ${ARGS_PLUGIN_NAME}
      CxPybind
  )

  target_compile_options(${PYTHON_MODULE_NAME}
    PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/MP>
  )

  set_target_properties(${PYTHON_MODULE_NAME}
    PROPERTIES
      ARCHIVE_OUTPUT_NAME ${PYTHON_MODULE_NAME}py
  )

  if(COMPLEX_PY_DISABLE_HIDDEN_VISIBILITY)
    set_target_properties(${PYTHON_MODULE_NAME}
      PROPERTIES
        CXX_VISIBILITY_PRESET default
    )
  endif()

  if(COMPLEX_PY_GENERATE_PYI)
    # complex_create_plugin_pyi(PLUGIN_NAME ${ARGS_PLUGIN_NAME})

    if(COMPLEX_PY_ENABLE_INSTALL)
      # install(FILES $<TARGET_FILE_DIR:complex>/${PYTHON_MODULE_NAME}.pyi
      #   DESTINATION ${COMPLEX_PY_INSTALL_DIR}
      # )
      install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${PYTHON_MODULE_NAME}.pyi
        DESTINATION ${COMPLEX_PY_INSTALL_DIR}
      )
    endif()
  endif()

  if(COMPLEX_PY_ENABLE_DOCS)
  endif()

  if(COMPLEX_PY_ENABLE_INSTALL)
    install(TARGETS ${PYTHON_MODULE_NAME}
      LIBRARY
        DESTINATION ${COMPLEX_PY_INSTALL_DIR}
    )
  endif()
endfunction()

if(COMPLEX_ENABLE_ITKImageProcessing)
  complex_add_python_plugin(PLUGIN_NAME ITKImageProcessing)
endif()

if(COMPLEX_ENABLE_OrientationAnalysis)
  complex_add_python_plugin(PLUGIN_NAME OrientationAnalysis)
endif()
