find_package(pybind11 2.10.0 CONFIG REQUIRED)

pybind11_add_module(complexpy complexpy.cpp)

# we want the CMake target name to be "complexpy", but the Python module to be "complex"
set_target_properties(complexpy
  PROPERTIES
    OUTPUT_NAME ${COMPLEX_PYTHON_MODULE}
    ARCHIVE_OUTPUT_NAME ${COMPLEX_PYTHON_MODULE}py
    LIBRARY_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:complex>
)

target_link_libraries(complexpy
  PUBLIC
    complex
    ComplexCore
)

target_compile_options(complexpy
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/MP>
)

install(TARGETS complexpy
  LIBRARY
    DESTINATION ${COMPLEX_PY_INSTALL_DIR}
)
