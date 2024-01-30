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

def check_filter_result(filter: nx.IFilter, result: nx.IFilter.ExecuteResult) -> None:
  if len(result.warnings) != 0:
    print(f'{filter.name()} ::  Warnings: {result.warnings}')
  
  has_errors = len(result.errors) != 0 
  if has_errors:
    print(f'{filter.name()} :: Errors: {result.errors}')
    raise RuntimeError(result)
  
  print(f"{filter.name()} :: No errors running the filter")

def get_build_directory() -> Path:
  return Path('${CMAKE_LIBRARY_OUTPUT_DIRECTORY}')

def get_test_directory() -> Path:
  return Path('${simplnx_BINARY_DIR}/Testing')

def get_test_temp_directory() -> Path:
  return Path('${simplnx_BINARY_DIR}/Testing/Temporary')

def get_data_directory() -> Path:
  return Path('${DATA_DEST_DIR}')

def get_simplnx_python_source_dir() -> Path:
  return Path('${simplnx_SOURCE_DIR}/wrapping/python')

def get_simplnx_source_dir() -> Path:
  return Path('${simplnx_SOURCE_DIR}')

def print_all_paths() -> None:
  print(f'##### Important Filesystem Paths #####')
  print(f'  get_build_directory:           {get_build_directory()}')
  print(f'  get_test_directory:            {get_test_directory()}')
  print(f'  get_test_temp_directory:       {get_test_temp_directory()}')
  print(f'  get_data_directory:            {get_data_directory()}')
  print(f'  get_simplnx_python_source_dir: {get_simplnx_python_source_dir()}')
  print(f'  get_simplnx_source_dir:        {get_simplnx_source_dir()}')
  print('#######################################')

def cleanup_test_file(file_path: str) -> None:
  if os.path.exists(file_path):
    os.remove(file_path)

  # Check if replace the extension with ".xdmf" the file exists and we can replace it
  filename = Path(file_path)
  filename_replace_ext = filename.with_suffix('.xdmf')
  if os.path.exists(filename_replace_ext):
    os.remove(filename_replace_ext)

def cleanup_test_dir(dir_path: str) -> None:
  if os.path.exists(dir_path):
    shutil.rmtree(dir_path)
