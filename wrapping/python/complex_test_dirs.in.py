''' module to get build specific directories
    This file is generated during the cmake configure phase. This file does *NOT*
    however use the standard `cmake_configure_file()` command but instead uses
    a custom generation cmake code. If you add new `${}` style variables, you will
    need to update those custom CMake codes.

 '''


import complex as cx

def check_filter_result(filter: cx.IFilter, result: cx.IFilter.ExecuteResult):
  if len(result.warnings) != 0:
    print(f'{filter.name()} ::  Warnings: {result.warnings}')
  
  has_errors = len(result.errors) != 0 
  if has_errors:
    print(f'{filter.name()} :: Errors: {result.errors}')
    raise RuntimeError(result)
  
  print(f"{filter.name()} :: No errors running the filter")


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

def print_all_paths():
    print(f'#### Important Filesystem Paths ####')
    print(f'  GetBuildDirectory:         {GetBuildDirectory()}')
    print(f'  GetTestDirectory:          {GetTestDirectory()}')
    print(f'  GetTestTempDirectory:      {GetTestTempDirectory()}')
    print(f'  GetDataDirectory:          {GetDataDirectory()}')
    print(f'  GetComplexPythonSourceDir: {GetComplexPythonSourceDir()}')
    print(f'  GetComplexSourceDir:       {GetComplexSourceDir()}')
    print('#######################################')