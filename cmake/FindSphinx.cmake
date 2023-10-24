get_filename_component(PYTHON_EXE_PATH "${Python3_EXECUTABLE}" DIRECTORY CACHE)
set(SPHINX_PYTHON_EXECUTABLE "${Python3_EXECUTABLE}")
find_program(SPHINX_BUILD_EXECUTABLE sphinx-build
    NAMES sphinx-build sphinx-build.exe
    PATHS "${PYTHON_EXE_PATH};${PYTHON_EXE_PATH}/Scripts")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Sphinx "Failed to find sphinx-build executable" SPHINX_BUILD_EXECUTABLE)
