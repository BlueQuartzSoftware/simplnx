''' module to get build specific directories
    This file is generated during the cmake configure phase. This file does *NOT*
    however use the standard `cmake_configure_file()` command but instead uses
    a custom generation cmake code. If you add new `${}` style variables, you will
    need to update those custom CMake codes.

 '''

import os
import shutil
from pathlib import Path

import simplnx as nx

def check_filter_result(filter: nx.IFilter, result: nx.IFilter.ExecuteResult):
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
  return 'C:/Users/Alex Jackson/Downloads/DREAM3DNX-7.0.0-RC-9-windows-AMD64/Data'

def GetSimplnxPythonSourceDir():
    return 'C:/Users/Alex Jackson/Workspace/simplnx/wrapping/python'

def GetSimplnxSourceDir():
    return 'C:/Users/Alex Jackson/Workspace/simplnx'


def print_all_paths():
    print(f'#### Important Filesystem Paths ####')
    print(f'  GetBuildDirectory:         {GetBuildDirectory()}')
    print(f'  GetTestDirectory:          {GetTestDirectory()}')
    print(f'  GetTestTempDirectory:      {GetTestTempDirectory()}')
    print(f'  GetDataDirectory:          {GetDataDirectory()}')
    print(f'  GetSimplnxPythonSourceDir: {GetSimplnxPythonSourceDir()}')
    print(f'  GetSimplnxSourceDir:       {GetSimplnxSourceDir()}')
    print('#######################################')

def cleanup_test_file(file_path: str):
  if os.path.exists(file_path):
    os.remove(file_path)

  # Check if replace the extension with ".xdmf" the file exists and we can replace it
  filename = Path(file_path)
  filename_replace_ext = filename.with_suffix('.xdmf')
  if os.path.exists(filename_replace_ext):
    os.remove(filename_replace_ext)

def cleanup_test_dir(dir_path: str):
  if os.path.exists(dir_path):
    shutil.rmtree(dir_path)