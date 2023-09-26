import json
import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple

SEPERATOR_PARAMETER_TYPE: str = "SeparatorFilterParameter"

@dataclass
class SIMPLParameterInfo:
  name: str
  type: str

  @staticmethod
  def from_json(json_object):
    return SIMPLParameterInfo(name=json_object['name'], type=json_object['type'])

@dataclass
class SIMPLFilterInfo:
  name: str
  uuid: str
  parameters: List[SIMPLParameterInfo]

  @staticmethod
  def from_json(json_object):
    parameters: List[SIMPLParameterInfo] = [SIMPLParameterInfo.from_json(param) for param in json_object['parameters']]
    stripped_uuid = json_object['uuid'].strip('{}')
    return SIMPLFilterInfo(name=json_object['name'], uuid=stripped_uuid, parameters=parameters)

def create_filter_conversion(simpl_filter: SIMPLFilterInfo, complex_filter_name: str) -> List[str]:
  converter_code: List[str] = []
  converter_code.append('\n')
  converter_code.append('namespace\n')
  converter_code.append('{\n')
  converter_code.append('namespace SIMPL\n')
  converter_code.append('{\n')

  for param in simpl_filter.parameters:
    if param.type == SEPERATOR_PARAMETER_TYPE:
      continue
    converter_code.append(f'constexpr StringLiteral k_{param.name}Key = "{param.name}";\n')

  converter_code.append('} // namespace SIMPL\n')
  converter_code.append('} // namespace\n')
  converter_code.append('\n')
  converter_code.append(f'Result<Arguments> {complex_filter_name}::FromSIMPLJson(const nlohmann::json& json)\n')
  converter_code.append('{\n')
  converter_code.append('  Arguments args = CreateDataArray().getDefaultArguments();\n')
  converter_code.append('\n')
  converter_code.append('  std::vector<Result<>> results;\n')
  converter_code.append('\n')

  for param in simpl_filter.parameters:
    if param.type == SEPERATOR_PARAMETER_TYPE:
      continue
    converter_code.append(f'  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::{param.type}Converter>(args, json, SIMPL::k_{param.name}Key, "@COMPLEX_PARAMETER_KEY@"));\n')

  converter_code.append('\n')
  converter_code.append('  Result<> conversionResult = MergeResults(std::move(results));\n')
  converter_code.append('\n')
  converter_code.append('  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));\n')
  converter_code.append('}\n')
  converter_code.append('\n')
  return converter_code

def create_includes() -> List[str]:
  lines: List[str] = []
  lines.append('\n')
  lines.append('#include "complex/Utilities/SIMPLConversion.hpp"\n')
  lines.append('\n')
  return lines

def create_function_decl() -> List[str]:
  lines: List[str] = []
  lines.append('\n')
  lines.append('  /**\n')
  lines.append('   * @brief Reads SIMPL json and converts it complex Arguments.\n')
  lines.append('   * @param json\n')
  lines.append('   * @return Result<Arguments>\n')
  lines.append('   */\n')
  lines.append('  static Result<Arguments> FromSIMPLJson(const nlohmann::json& json);\n')
  lines.append('\n')
  return lines

def find_last_include(lines: List[str]) -> int:
  index = -1
  for i, line in enumerate(lines):
    if line.startswith('#include'):
      index = i
  return index

def find_last_parameter_key(lines: List[str]) -> int:
  index = -1
  for i, line in enumerate(lines):
    if line.startswith('  static inline constexpr StringLiteral k_'):
      index = i
  return index

def find_mapping_line(lines: List[str], simpl_uuid: str) -> int:
  for i, line in enumerate(lines):
    if simpl_uuid in line:
      return i
  return -1

def read_mapping_file(filepath: Path) -> Dict[str, str]:
  with open(filepath, 'r') as file:
    file_content = file.read()
    matches = re.findall(r'{complex::Uuid::FromString\("(.*?)"\).value\(\), {complex::FilterTraits<(.*?)>::uuid, {}}},', file_content)
    return dict(matches)

def get_plugin_mapping_file_path_from_plugin_dir(plugin_dir: Path, plugin_name: str) -> Path:
  return plugin_dir / f'src/{plugin_name}/{plugin_name}LegacyUUIDMapping.hpp'

def get_plugin_mapping_file_path_from_root_dir(complex_source_dir: Path, plugin_name: str) -> Path:
  return get_plugin_mapping_file_path_from_plugin_dir(complex_source_dir / f'src/Plugins/{plugin_name}', plugin_name)

def get_plugin_mappings(complex_source_dir: Path) -> Dict[str, Dict[str, str]]:
  mappings: Dict[str, Dict[str, str]] = {}
  plugins_dir = complex_source_dir / 'src/Plugins'
  ignored_plugins = ['TestOne', 'TestTwo']
  for child in plugins_dir.iterdir():
    if not child.is_dir():
      continue
    if child.name in ignored_plugins:
      continue
    plugin_name = child.name
    mapping_file = get_plugin_mapping_file_path_from_plugin_dir(child, plugin_name)
    plugin_mapping = read_mapping_file(mapping_file)
    mappings[plugin_name] = plugin_mapping
  return mappings

def find_filter(mappings: Dict[str, Dict[str, str]], filter_uuid: str) -> Tuple[str, str]:
  for plugin_name, plugin_mapping in mappings.items():
    if filter_uuid in plugin_mapping:
      return (plugin_name, plugin_mapping[filter_uuid])
  raise RuntimeError(f'{filter_uuid} not found')

def get_filter_base_path(complex_source_dir: Path, plugin_name: str, complex_filter: str) -> Path:
  return complex_source_dir / f'src/Plugins/{plugin_name}/src/{plugin_name}/Filters/{complex_filter}'

def get_filter_hpp_path(complex_source_dir: Path, plugin_name: str, complex_filter: str) -> Path:
  return get_filter_base_path(complex_source_dir, plugin_name, complex_filter).with_suffix('.hpp')

def get_filter_cpp_path(complex_source_dir: Path, plugin_name: str, complex_filter: str) -> Path:
  return get_filter_base_path(complex_source_dir, plugin_name, complex_filter).with_suffix('.cpp')

def read_simpl_json(path: Path) -> Dict[str, SIMPLFilterInfo]:
  with open(path, 'r') as file:
    simpl_json = json.load(file)

  simpl_filters: Dict[str, SIMPLFilterInfo] = {}
  for plugin in simpl_json['plugins']:
    for f in plugin['filters']:
      filter_info = SIMPLFilterInfo.from_json(f)
      simpl_filters[filter_info.uuid] = filter_info
  return simpl_filters

def update_hpp_lines(lines: List[str]) -> None:
  last_parameter_key_index = find_last_parameter_key(lines)

  function_decl_lines = create_function_decl()
  lines[last_parameter_key_index:last_parameter_key_index] = function_decl_lines

def update_cpp_lines(lines: List[str], simpl_filter_info: SIMPLFilterInfo, complex_filter_name: str) -> None:
  last_include_index = find_last_include(lines)

  include_lines = create_includes()

  lines[last_include_index:last_include_index] = include_lines

  filter_conversion_lines = create_filter_conversion(simpl_filter_info, complex_filter_name)

  lines.extend(filter_conversion_lines)

def update_filter_hpp(complex_filter_path: Path) -> None:
  with open(complex_filter_path, 'r') as input_file:
    lines = input_file.readlines()

  update_hpp_lines(lines)

  with open(complex_filter_path, 'w') as output_file:
    output_file.writelines(lines)

def update_filter_cpp(complex_filter_path: Path, simpl_filter_info: SIMPLFilterInfo, complex_filter_name: str) -> None:
  with open(complex_filter_path, 'r') as input_file:
    lines = input_file.readlines()

  update_cpp_lines(lines, simpl_filter_info, complex_filter_name)

  with open(complex_filter_path, 'w') as output_file:
    output_file.writelines(lines)

def update_mapping_lines(lines: List[str], simpl_uuid: str, complex_filter_name: str) -> None:
  index = find_mapping_line(lines, simpl_uuid)
  lines[index] = lines[index].replace('{}', f'&{complex_filter_name}::FromSIMPLJson')

def update_mapping_file(mapping_file_path: Path, simpl_uuid: str, complex_filter_name: str) -> None:
  with open(mapping_file_path, 'r') as input_file:
    lines = input_file.readlines()

  update_mapping_lines(lines, simpl_uuid, complex_filter_name)

  with open(mapping_file_path, 'w') as output_file:
    output_file.writelines(lines)

def generate_converter_code(complex_source_dir: Path, simpl_json_path: Path, simpl_filters: List[str]) -> None:
  mappings = get_plugin_mappings(complex_source_dir)
  simpl_filters_info = read_simpl_json(simpl_json_path)
  for simpl_filter_uuid in simpl_filters:
    if simpl_filter_uuid not in simpl_filters_info:
      raise RuntimeError(f'SIMPL filter json does not contain {simpl_filter_uuid}')
    plugin_name, complex_filter_name = find_filter(mappings, simpl_filter_uuid)
    mapping_file_path = get_plugin_mapping_file_path_from_root_dir(complex_source_dir, plugin_name)
    complex_filter_hpp_path = get_filter_hpp_path(complex_source_dir, plugin_name, complex_filter_name)
    complex_filter_cpp_path = get_filter_cpp_path(complex_source_dir, plugin_name, complex_filter_name)
    update_filter_hpp(complex_filter_hpp_path)
    update_filter_cpp(complex_filter_cpp_path, simpl_filters_info[simpl_filter_uuid], complex_filter_name)
    update_mapping_file(mapping_file_path, simpl_filter_uuid, complex_filter_name)

# e.g. python generate_simpl_conversion_code.py . --simpl-filters "53df5340-f632-598f-8a9b-802296b3a95c"
# simpl-json is assumed to be next to this file, but can be overriden

def main() -> None:
  parser = argparse.ArgumentParser()
  parser.add_argument('complex_source_dir', type=Path)
  parser.add_argument('--simpl-json', type=Path, default=Path('./simpl_filters.json'))
  parser.add_argument('--simpl-filters', nargs='+')

  args = parser.parse_args()

  complex_source_dir: Path = args.complex_source_dir
  simpl_json: Path = args.simpl_json
  simpl_filters: List[str] = args.simpl_filters

  generate_converter_code(complex_source_dir.absolute(), simpl_json.absolute(), simpl_filters)

if __name__ == '__main__':
  main()
