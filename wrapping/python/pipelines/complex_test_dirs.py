''' module to get build specific directories
    This file is generated during the cmake configure phase. This file does *NOT*
    however use the standard `cmake_configure_file()` command but instead uses
    a custom generation cmake code. If you add new `${}` style variables, you will
    need to update those custom CMake codes.

    The prospective developer should substitute the variables in ALL_CAPS with appropriate
    directory paths
 '''

def GetBuildDirectory():
  return '${CMAKE_LIBRARY_OUTPUT_DIRECTORY}'

def GetTestDirectory():
  return '${complex_BINARY_DIR}/Testing'

def GetTestTempDirectory():
  return '${complex_BINARY_DIR}/Testing/Temporary'

def GetDataDirectory():
  return '${DREAM3D_DATA_DIR}'

def GetComplexPythonSourceDir():
    return '${complex_SOURCE_DIR}/wrapping/python'

def GetComplexSourceDir():
    return '${complex_SOURCE_DIR}'
