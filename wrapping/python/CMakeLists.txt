find_package(pybind11 2.10.0 CONFIG REQUIRED)

pybind11_add_module(complexpy complexpy.cpp)

set(COMPLEX_PYTHON_MODULE "complex")

# we want the CMake target name to be "complexpy", but the Python module to be "complex"
set_target_properties(complexpy
  PROPERTIES
    OUTPUT_NAME ${COMPLEX_PYTHON_MODULE}
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
