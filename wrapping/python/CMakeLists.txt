find_package(pybind11 2.10.0 CONFIG REQUIRED)

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
    complex
    ComplexCore
)

target_compile_options(complexpy
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

option(COMPLEX_PY_GENERATE_PYI "Generate .pyi files for python bindings" OFF)
if(COMPLEX_PY_GENERATE_PYI)
  find_program(MYPY_STUBGEN_EXE stubgen REQUIRED)

  add_custom_target(complexCreateStubFile ALL
    COMMAND ${MYPY_STUBGEN_EXE} -m complex -o $<TARGET_FILE_DIR:complex>
    COMMENT "complex: Generating .pyi files"
    # BYPRODUCTS $<TARGET_FILE_DIR:complex>/complex.pyi
    WORKING_DIRECTORY $<TARGET_FILE_DIR:complex>
  )
  set_property(TARGET complexCreateStubFile PROPERTY FOLDER "Python/Stub")
  add_dependencies(complexCreateStubFile complexpy)
endif()

option(COMPLEX_PY_ENABLE_INSTALL "Enables install complexpy" OFF)
if(COMPLEX_PY_ENABLE_INSTALL)
  include(GNUInstallDirs)
  set(COMPLEX_PY_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR} CACHE PATH "Path to install complexpy")

  install(TARGETS complexpy
    LIBRARY
      DESTINATION ${COMPLEX_PY_INSTALL_DIR}
  )
endif()
