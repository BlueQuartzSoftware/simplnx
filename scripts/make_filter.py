import argparse
import uuid

from pathlib import Path

def make_filter(plugin_dir: Path, name: str, template_dir: Path) -> None:

  plugin_name = plugin_dir.name
  filter_name = f'{name}Filter'
  header_template_file = f'{template_dir}/simplnx_filter.hpp.in'
  header_file_contents: str
  with open(header_template_file, 'r') as header_file:
    header_file_contents = header_file.read().replace('@FILTER_NAME@', filter_name).replace('@UUID@', str(uuid.uuid4()))
    header_file_contents = header_file_contents.replace('@PLUGIN_NAME_UPPER@', plugin_name.upper())
    header_file_contents = header_file_contents.replace('@PLUGIN_NAME@', plugin_name)
    header_file_contents = header_file_contents.replace('@PARAMETER_KEYS@', "  // THESE NEED TO BE GENERATED\n")

  # write contents out to the new target header file
  header_target_file = f'{plugin_dir}/src/{plugin_name}/Filters/{name}Filter.hpp'
  print(f'Writing Header file: {header_target_file}')
  with open(header_target_file, 'w') as header_file:
    header_file.write(header_file_contents)

  cpp_template_file = f'{template_dir}/simplnx_filter.cpp.in'
  cpp_file_contents: str
  with open(cpp_template_file, 'r') as cpp_file:
    cpp_file_contents = cpp_file.read().replace('@ALGORITHM_NAME@', name)
    cpp_file_contents = cpp_file_contents.replace('@FILTER_NAME@', filter_name)
    cpp_file_contents = cpp_file_contents.replace('@PLUGIN_NAME@', plugin_name)
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_KEYS@', "  //TODO: THESE NEED TO BE GENERATED\n")
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_INCLUDES@', "\n //TODO: PARAMETER_INCLUDES")
    cpp_file_contents = cpp_file_contents.replace('@DEFAULT_TAGS@', "\"\"")
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_DEFS@', "\n //TODO: PARAMETER_DEFS")
    cpp_file_contents = cpp_file_contents.replace('@PREFLIGHT_DEFS@', "\n //TODO: PREFLIGHT_DEFS")
    cpp_file_contents = cpp_file_contents.replace('@PROPOSED_ACTIONS@', "\n //TODO: PROPOSED_ACTIONS")
    cpp_file_contents = cpp_file_contents.replace('@PREFLIGHT_UPDATED_DEFS@', "\n //TODO: PREFLIGHT_UPDATED_DEFS")    
    cpp_file_contents = cpp_file_contents.replace('@PREFLIGHT_UPDATED_VALUES@', "\n //TODO: PREFLIGHT_UPDATED_VALUES")
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_JSON_CONSTANTS@', "\n //TODO: PARAMETER_JSON_CONSTANTS")
    cpp_file_contents = cpp_file_contents.replace('@INPUT_VALUES_DEF@', "\n //TODO: INPUT_VALUES_DEF")
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_JSON_CONVERSION@', "/* This is a NEW filter and not ported so this section does not matter */")

  # write contents out to the new target CPP file
  cpp_target_file = f'{plugin_dir}/src/{plugin_name}/Filters/{name}Filter.cpp'
  print(f'Writing CPP file:    {cpp_target_file}')
  with open(cpp_target_file, 'w') as cpp_file:
    cpp_file.write(cpp_file_contents)

  # ***************************************************************************
  # Algorithm File Generation
  # ***************************************************************************
  header_template_file = f'{template_dir}/simplnx_algorithm.hpp.in'
  header_file_contents: str
  with open(header_template_file, 'r') as header_file:
    header_file_contents = header_file.read().replace('@FILTER_NAME@', name).replace('@UUID@', str(uuid.uuid4()))
    header_file_contents = header_file_contents.replace('@PLUGIN_NAME_UPPER@', plugin_name.upper())
    header_file_contents = header_file_contents.replace('@PLUGIN_NAME@', plugin_name)
    header_file_contents = header_file_contents.replace('@PARAMETER_KEYS@', "  // THESE NEED TO BE GENERATED\n")    
    header_file_contents = header_file_contents.replace('@PARAMETER_INCLUDES@', "\n//TODO: PARAMETER_INCLUDES")
    header_file_contents = header_file_contents.replace('@INPUT_VALUE_STRUCT_DEF@', "//TODO: INPUT_VALUE_STRUCT_DEF\n")

  # write contents out to the new target header file
  header_target_file = f'{plugin_dir}/src/{plugin_name}/Filters/Algorithms/{name}.hpp'
  print(f'Writing Header file: {header_target_file}')
  with open(header_target_file, 'w') as header_file:
    header_file.write(header_file_contents)  

  cpp_template_file = f'{template_dir}/simplnx_algorithm.cpp.in'
  cpp_file_contents: str
  with open(cpp_template_file, 'r') as cpp_file:
    cpp_file_contents = cpp_file.read().replace('@FILTER_NAME@', name)

  # write contents out to the new target CPP file
  cpp_target_file = f'{plugin_dir}/src/{plugin_name}/Filters/Algorithms/{name}.cpp'
  print(f'Writing CPP file:    {cpp_target_file}')
  with open(cpp_target_file, 'w') as cpp_file:
    cpp_file.write(cpp_file_contents)

  # ***************************************************************************
  # Unit test File Generation
  # ***************************************************************************
  cpp_template_file = f'{template_dir}/simplnx_unit_test.cpp.in'
  cpp_file_contents: str
  with open(cpp_template_file, 'r') as cpp_file:
    cpp_file_contents = cpp_file.read().replace('@FILTER_NAME@', filter_name)
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_INCLUDES@', "\n //TODO: PARAMETER_INCLUDES")
    cpp_file_contents = cpp_file_contents.replace('@PLUGIN_NAME@', plugin_name)
    cpp_file_contents = cpp_file_contents.replace('@PARAMETER_DEFS@', "\n //TODO: PARAMETER_DEFS")

  # write contents out to the new target CPP file
  cpp_target_file = f'{plugin_dir}/test/{name}Test.cpp'
  print(f'Writing Doc file:    {cpp_target_file}')
  with open(cpp_target_file, 'w') as cpp_file:
    cpp_file.write(cpp_file_contents)


  # ***************************************************************************
  # Documentation File Generation
  # ***************************************************************************
  cpp_template_file = f'{template_dir}/simplnx_docs.md.in'
  cpp_file_contents: str
  with open(cpp_template_file, 'r') as cpp_file:
    cpp_file_contents = cpp_file.read().replace('@FILTER_NAME@', name)

  # write contents out to the new target CPP file
  cpp_target_file = f'{plugin_dir}/docs/{name}Filter.md'
  print(f'Writing Doc file:    {cpp_target_file}')
  with open(cpp_target_file, 'w') as cpp_file:
    cpp_file.write(cpp_file_contents)



  print(f'===================================================================')
  print(f'Do NOT forget to add your filter and algorithm to the CMakeLists.txt file at:')
  print(f'{plugin_dir}/CMakeLists.txt')
  print(f'Update the unit test CMakeLists.txt file at:')
  print(f'{plugin_dir}/test/CMakeLists.txt')
  print(f'Do NOT forget to update the documentation file when you are done writing the file')
  print(f'===================================================================')

def main() -> None:
  parser = argparse.ArgumentParser(description='Creates simplnx filter header and implementation skeleton codes')
  parser.add_argument('-o', '--plugin_dir', type=Path, help='Plugin Directory to create the filter')
  parser.add_argument('-n', '--name', type=str, help='Name of filter')
  parser.add_argument('-t', '--template_dir', type=Path, help='Location of template files')

  args = parser.parse_args()

  print('args:')
  print(f'  plugin_dir = \"{args.plugin_dir}\"')
  print(f'  name = \"{args.name}\"')
  print(f'  template_dir = \"{args.template_dir}\"')
  print('')

  print(f'===================================================================')
  print(f'        THIS WILL OVERWRITE ANY EXISTING FILE')
  print(f'===================================================================')

  make_filter(args.plugin_dir, args.name, args.template_dir)

if __name__ == '__main__':
  main()

# -----------------------------------------------------------------------------
# Example invocation
# python make_filter.py --plugin_dir /Users/mjackson/Workspace6/simplnx/src/Plugins/SimplnxCore --name "ReadNotesFile" --template_dir /Users/mjackson/Workspace6/simplnx/scripts
# -----------------------------------------------------------------------------
