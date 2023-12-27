''' module to get build specific directories
    This file is generated during the cmake configure phase. This file does *NOT*
    however use the standard `cmake_configure_file()` command but instead uses
    a custom generation cmake code. If you add new `${}` style variables, you will
    need to update those custom CMake codes.

 '''


<<<<<<<< HEAD:wrapping/python/simplnx_test_dirs.in.py
import simplnx as sx
========
import simplnx as cx
>>>>>>>> cf84ef01a (Various naming updates and organization changes):wrapping/python/testing/simplnx_test_dirs.in.py

def check_filter_result(filter: sx.IFilter, result: sx.IFilter.ExecuteResult):
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
  return '${simplnx_BINARY_DIR}/Testing'

def GetTestTempDirectory():
  return '${simplnx_BINARY_DIR}/Testing/Temporary'

def GetDataDirectory():
  return '${DREAM3D_DATA_DIR}'

<<<<<<<< HEAD:wrapping/python/simplnx_test_dirs.in.py
def GetSimplnxPythonSourceDir():
    return '${simplnx_SOURCE_DIR}/wrapping/python'

def GetSimplnxSourceDir():
========
def GetsimplnxPythonSourceDir():
    return '${simplnx_SOURCE_DIR}/wrapping/python'

def GetsimplnxSourceDir():
>>>>>>>> cf84ef01a (Various naming updates and organization changes):wrapping/python/testing/simplnx_test_dirs.in.py
    return '${simplnx_SOURCE_DIR}'

def print_all_paths():
    print(f'#### Important Filesystem Paths ####')
    print(f'  GetBuildDirectory:         {GetBuildDirectory()}')
    print(f'  GetTestDirectory:          {GetTestDirectory()}')
    print(f'  GetTestTempDirectory:      {GetTestTempDirectory()}')
    print(f'  GetDataDirectory:          {GetDataDirectory()}')
<<<<<<<< HEAD:wrapping/python/simplnx_test_dirs.in.py
    print(f'  GetSimplnxPythonSourceDir: {GetSimplnxPythonSourceDir()}')
    print(f'  GetSimplnxSourceDir:       {GetSimplnxSourceDir()}')
========
    print(f'  GetsimplnxPythonSourceDir: {GetsimplnxPythonSourceDir()}')
    print(f'  GetsimplnxSourceDir:       {GetsimplnxSourceDir()}')
>>>>>>>> cf84ef01a (Various naming updates and organization changes):wrapping/python/testing/simplnx_test_dirs.in.py
    print('#######################################')