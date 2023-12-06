find_package(pybind11 2.10.0 CONFIG REQUIRED)

add_library(CxPybind INTERFACE)

target_include_directories(CxPybind INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/CxPybind)

target_link_libraries(CxPybind
  INTERFACE
    simplnx
    pybind11::headers
)

pybind11_add_module(simplnxpy "${SimplnxCore_SOURCE_DIR}/wrapping/python/simplnxpy.cpp")

target_compile_definitions(simplnxpy PUBLIC PYBIND11_DETAILED_ERROR_MESSAGES)

# we want the CMake target name to be "simplnxpy", but the Python module to be "simplnx"
set_target_properties(simplnxpy
  PROPERTIES
    OUTPUT_NAME ${SIMPLNX_PYTHON_MODULE}
    ARCHIVE_OUTPUT_NAME ${SIMPLNX_PYTHON_MODULE}py
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:simplnx>
)

# pybind11 sets CXX_VISIBILITY_PRESET to hidden (though this may change in the future see https://github.com/pybind/pybind11/pull/4072)
# This can cause issues when using libc++ and std::any across shared libraries
# The hidden visibility causes some objects to have different RTTI which then results in std::bad_any_cast
option(SIMPLNX_PY_DISABLE_HIDDEN_VISIBILITY "Controls CXX_VISIBILITY_PRESET for python modules" OFF)

if(SIMPLNX_PY_DISABLE_HIDDEN_VISIBILITY)
  set_target_properties(simplnxpy
    PROPERTIES
      CXX_VISIBILITY_PRESET default
  )
endif()

target_link_libraries(simplnxpy
  PUBLIC
    SimplnxCore
    CxPybind
)

target_compile_options(simplnxpy
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

#-------------------------------------------------------------------------------
# FUNCTION: simplnx_generate_filter_bindings
# ARGS:
#   One Value:
#      CPP_PLUGIN_NAME:
#      PYTHON_MODULE_NAME:
#   Multi Value:
#      FILTER_LIST
#-------------------------------------------------------------------------------
function(simplnx_generate_filter_bindings)
  set(options)
  set(oneValueArgs CPP_PLUGIN_NAME PYTHON_MODULE_NAME)
  set(multiValueArgs FILTER_LIST)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(PLUGIN_NAME ${ARGS_CPP_PLUGIN_NAME})
  set(FILTER_INCLUDE_CODE "")
  set(FILTER_BINDING_CODE "")
  
  foreach(FILTER IN LISTS ARGS_FILTER_LIST)
    string(APPEND FILTER_INCLUDE_CODE "#include <${PLUGIN_NAME}/Filters/${FILTER}.hpp>\n")
    string(APPEND FILTER_BINDING_CODE "  BindFilter<${FILTER}>(scope, internals);\n")
  endforeach()

  set(GENERATED_PYTHON_DIR ${PROJECT_BINARY_DIR}/python/generated/${PLUGIN_NAME})

  set(GENERATED_FILTER_BINDING_HPP ${GENERATED_PYTHON_DIR}/${PLUGIN_NAME}/${PLUGIN_NAME}FilterBinding.hpp)
  set(GENERATED_FILTER_BINDING_CPP ${GENERATED_PYTHON_DIR}/${PLUGIN_NAME}/${PLUGIN_NAME}FilterBinding.cpp)

  configure_file(${PROJECT_SOURCE_DIR}/wrapping/python/cmake/FilterBinding.hpp.in ${GENERATED_FILTER_BINDING_HPP} @ONLY)
  configure_file(${PROJECT_SOURCE_DIR}/wrapping/python/cmake/FilterBinding.cpp.in ${GENERATED_FILTER_BINDING_CPP} @ONLY)

  target_sources(${ARGS_PYTHON_MODULE_NAME}
    PRIVATE
      ${GENERATED_FILTER_BINDING_HPP}
      ${GENERATED_FILTER_BINDING_CPP}
  )

  target_include_directories(${ARGS_PYTHON_MODULE_NAME}
    PRIVATE
      ${GENERATED_PYTHON_DIR}
  )
endfunction()

#-------------------------------------------------------------------------------
# FUNCTION: simplnx_create_pyi
# ARGS:
#   One Value:
#      PYTHON_MODULE_NAME:
#      PYTHON_TARGET:
#   Multi Value:
#      
#-------------------------------------------------------------------------------
function(simplnx_create_pyi)
  set(options)
  set(oneValueArgs PYTHON_MODULE_NAME PYTHON_TARGET)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  find_program(MYPY_STUBGEN_EXE stubgen REQUIRED)

  set(PYI_TARGET_NAME ${ARGS_PYTHON_MODULE_NAME}CreateStubFile)

  add_custom_target(${PYI_TARGET_NAME} ALL
    COMMAND ${MYPY_STUBGEN_EXE} -m ${ARGS_PYTHON_MODULE_NAME} -o $<TARGET_FILE_DIR:simplnx>
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/fix_pyi_file.py --input_file "$<TARGET_FILE_DIR:simplnx>/${ARGS_PYTHON_MODULE_NAME}.pyi"
    COMMENT "${ARGS_PYTHON_MODULE_NAME}: Generating .pyi files"
    WORKING_DIRECTORY $<TARGET_FILE_DIR:simplnx>
  )
  set_property(TARGET ${PYI_TARGET_NAME} PROPERTY FOLDER "Python/Stub")
  add_dependencies(${PYI_TARGET_NAME} ${ARGS_PYTHON_TARGET})
endfunction()

#-------------------------------------------------------------------------------
# FUNCTION: simplnx_create_plugin_pyi
# ARGS:
#   One Value:
#      PLUGIN_NAME:
#   Multi Value:
#      
#-------------------------------------------------------------------------------
function(simplnx_create_plugin_pyi)
  set(options)
  set(oneValueArgs PLUGIN_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  string(TOLOWER ${ARGS_PLUGIN_NAME} PLUGIN_NAME_LOWER)

  simplnx_create_pyi(PYTHON_MODULE_NAME ${PLUGIN_NAME_LOWER} PYTHON_TARGET ${PLUGIN_NAME_LOWER})
  add_dependencies(${PLUGIN_NAME_LOWER} simplnxpy)
endfunction()

get_property(SIMPLNX_CORE_FILTER_LIST TARGET SimplnxCore PROPERTY SIMPLNX_FILTER_LIST)

simplnx_generate_filter_bindings(CPP_PLUGIN_NAME SimplnxCore
  PYTHON_MODULE_NAME simplnxpy
  FILTER_LIST ${SIMPLNX_CORE_FILTER_LIST}
)

set_property(TARGET simplnxpy PROPERTY FOLDER "Python")

option(SIMPLNX_PY_GENERATE_PYI "Generate .pyi files for python bindings" OFF)
if(SIMPLNX_PY_GENERATE_PYI)
  simplnx_create_pyi(PYTHON_MODULE_NAME simplnx PYTHON_TARGET simplnxpy)
endif()

# Disabled due to auto generation not fully working
# option(SIMPLNX_PY_ENABLE_DOCS "Generate python documentation using sphinx" OFF)
# if(SIMPLNX_PY_ENABLE_DOCS)
#   find_package(Sphinx REQUIRED)

#   set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/docs)
#   set(SPHINX_BUILD ${PROJECT_BINARY_DIR}/docs/python)

#   add_custom_target(simplnxpy_docs
#     COMMAND
#       ${CMAKE_COMMAND} -E env "PYTHONPATH=$<TARGET_FILE_DIR:simplnx>"
#       ${SPHINX_BUILD_EXECUTABLE} -b html ${SPHINX_SOURCE} ${SPHINX_BUILD}
#     WORKING_DIRECTORY $<TARGET_FILE_DIR:simplnx>
#     COMMENT "Generating simplnxpy docs with Sphinx"
#     VERBATIM
#   )
#   add_dependencies(simplnxpy_docs simplnxCreateStubFile)
# endif()

option(SIMPLNX_PY_ENABLE_INSTALL "Enables install simplnxpy" OFF)
if(SIMPLNX_PY_ENABLE_INSTALL)
  include(GNUInstallDirs)
  set(SIMPLNX_PY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Path to install simplnxpy")

  install(TARGETS simplnxpy
    LIBRARY
      DESTINATION ${SIMPLNX_PY_INSTALL_DIR}
  )
  if(SIMPLNX_PY_GENERATE_PYI)
    install(FILES $<TARGET_FILE_DIR:simplnx>/simplnx.pyi
      DESTINATION ${SIMPLNX_PY_INSTALL_DIR}
    )
  endif()
endif()


#-------------------------------------------------------------------------------
# FUNCTION: simplnx_add_python_plugin
# ARGS:
#   One Value:
#      PLUGIN_NAME:
#   Multi Value:
#      
#-------------------------------------------------------------------------------
function(simplnx_add_python_plugin)
  set(options)
  set(oneValueArgs PLUGIN_NAME PYTHON_MODULE_NAME)
  set(multiValueArgs)
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  set(PYTHON_MODULE_NAME ${ARGS_PYTHON_MODULE_NAME})
  set(PYTHON_WRAPPING_DIR "${${ARGS_PLUGIN_NAME}_SOURCE_DIR}/wrapping/python")

  pybind11_add_module(${PYTHON_MODULE_NAME} ${PYTHON_WRAPPING_DIR}/${PYTHON_MODULE_NAME}.cpp)

  get_property(FILTER_LIST TARGET ${ARGS_PLUGIN_NAME} PROPERTY SIMPLNX_FILTER_LIST)

  simplnx_generate_filter_bindings(CPP_PLUGIN_NAME ${ARGS_PLUGIN_NAME}
    PYTHON_MODULE_NAME ${PYTHON_MODULE_NAME}
    FILTER_LIST ${FILTER_LIST}
  )

  target_link_libraries(${PYTHON_MODULE_NAME}
    PUBLIC
      ${ARGS_PLUGIN_NAME}
      CxPybind
  )

  target_compile_definitions(${PYTHON_MODULE_NAME} PUBLIC PYBIND11_DETAILED_ERROR_MESSAGES)

  target_compile_options(${PYTHON_MODULE_NAME}
    PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/MP>
  )

  set_target_properties(${PYTHON_MODULE_NAME}
    PROPERTIES
      ARCHIVE_OUTPUT_NAME ${PYTHON_MODULE_NAME}py
  )

  if(SIMPLNX_PY_DISABLE_HIDDEN_VISIBILITY)
    set_target_properties(${PYTHON_MODULE_NAME}
      PROPERTIES
        CXX_VISIBILITY_PRESET default
    )
  endif()

  set_property(TARGET ${PYTHON_MODULE_NAME} PROPERTY FOLDER "Python")

  if(SIMPLNX_PY_GENERATE_PYI)
    simplnx_create_plugin_pyi(PLUGIN_NAME ${ARGS_PLUGIN_NAME})

    if(SIMPLNX_PY_ENABLE_INSTALL)
      install(FILES $<TARGET_FILE_DIR:simplnx>/${PYTHON_MODULE_NAME}.pyi
        DESTINATION ${SIMPLNX_PY_INSTALL_DIR}
      )
    endif()
  endif()

  if(SIMPLNX_PY_ENABLE_INSTALL)
    install(TARGETS ${PYTHON_MODULE_NAME}
      LIBRARY
        DESTINATION ${SIMPLNX_PY_INSTALL_DIR}
    )
  endif()
endfunction()

get_property(simplnxPluginTargets GLOBAL PROPERTY simplnxPluginTargets)
foreach(d3dPlugin ${simplnxPluginTargets})
  # message(STATUS "PYTHON: Checking Plugin ${d3dPlugin}")
  string(TOLOWER ${d3dPlugin} python_module_name)
  if(NOT "${d3dPlugin}" STREQUAL "SimplnxCore")
    if(SIMPLNX_PLUGIN_ENABLE_${d3dPlugin})
      message(STATUS "PYTHON: Wrapping Plugin ${d3dPlugin}")
      simplnx_add_python_plugin(PLUGIN_NAME ${d3dPlugin} PYTHON_MODULE_NAME ${python_module_name})
    endif()
  endif()
endforeach()

#------------------------------------------------------------------------------
# Create the Sphinx based Documentation
#------------------------------------------------------------------------------
option(SIMPLNX_ENABLE_SPHINX_DOCS "Enables the Sphinx based documentation generation" OFF)
if(SIMPLNX_ENABLE_SPHINX_DOCS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/docs ${simplnx_BINARY_DIR}/sphinx_docs)
endif()

#------------------------------------------------------------------------------
# Create the Python Unit tests
#------------------------------------------------------------------------------
option(COMPLEX_ENABLE_PYTHON_TESTS "Enables python based unit tests" OFF)
if(COMPLEX_ENABLE_PYTHON_TESTS)

    file(READ ${complex_SOURCE_DIR}/wrapping/python/complex_test_dirs.in.py COMPLEX_TEST_DIRS_FILE)

    string(REPLACE "\${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" $<TARGET_FILE_DIR:complex> COMPLEX_TEST_DIRS_FILE ${COMPLEX_TEST_DIRS_FILE})
    string(REPLACE "\${complex_BINARY_DIR}" ${complex_BINARY_DIR} COMPLEX_TEST_DIRS_FILE ${COMPLEX_TEST_DIRS_FILE})
    string(REPLACE "\${DREAM3D_DATA_DIR}" ${DREAM3D_DATA_DIR} COMPLEX_TEST_DIRS_FILE ${COMPLEX_TEST_DIRS_FILE})
    string(REPLACE "\${complex_SOURCE_DIR}" ${complex_SOURCE_DIR} COMPLEX_TEST_DIRS_FILE ${COMPLEX_TEST_DIRS_FILE})

    file(GENERATE
      OUTPUT "$<TARGET_FILE_DIR:complex>/complex_test_dirs.py"
      CONTENT ${COMPLEX_TEST_DIRS_FILE}
    )

    set(PYTHON_TEST_INPUT_DIR "${complex_SOURCE_DIR}/wrapping/python/examples")

    set(COMPLEX_PYTHON_TESTS
      "basic_ebsd_ipf"
      "import_d3d" 
      # "angle_conversion" 
      # "basic_arrays" 
      # "basic_numpy" 
      # "create_ensemble_info" 
      # "generated_file_list" 
      # "geometry_examples" 
      # "import_hdf5" 
      # "output_file" 
      # "pipeline" 
      # "read_csv_file"
    )

    CreatePythonTests(PREFIX "PY_COMPLEX"
      INPUT_DIR ${PYTHON_TEST_INPUT_DIR}
      TEST_NAMES ${COMPLEX_PYTHON_TESTS}
    )

endif()
