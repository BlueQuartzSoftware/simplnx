#!/usr/bin/env python3
"""
Generates SIMPL backwards compatibility test fixtures and test code for NX filters.

For each filter that has a FromSIMPLJson() method:
  1. Parses the SIMPL namespace constants and ConvertParameter calls
  2. Looks up the SIMPL UUID from the plugin's LegacyUUIDMapping
  3. Looks up the Filter_Name from LegacySimplFilterUuid.hpp
  4. Generates two JSON fixture files (6_5 with UUID, 6_4 without)
  5. Generates a TEST_CASE and appends it to the filter's test .cpp file
"""

import os
import re
import json
import logging
import glob as globmod
from pathlib import Path
from typing import Optional

logging.basicConfig(level=logging.INFO, format="%(levelname)s: %(message)s")
logger = logging.getLogger(__name__)

ROOT = Path("/Users/mjackson/Workspace3/simplnx")

# Filters that already have backwards compatibility tests - skip these
SKIP_FILTERS = {
    "ReadAngDataFilter",
    "ReadCtfDataFilter",
    "ScalarSegmentFeaturesFilter",
    "ComputeTriangleAreasFilter",
}

# Converter types that should be skipped entirely (complex custom types)
SKIP_CONVERTERS = {
    "DataContainerArrayProxyFilterParameterConverter",
    "ComparisonSelectionFilterParameterConverter",
    "ComparisonSelectionAdvancedFilterParameterConverter",
    "ReadASCIIDataFilterParameterConverter",
    "ImportHDF5DatasetFilterParameterConverter",
    "DataContainerReaderFilterParameterConverter",
    "MultiAttributeMatrixSelectionFilterParameterConverter",
    "MultiDataContainerSelectionFilterParameterConverter",
}

# Default JSON values for SIMPL parameters based on converter type
SIMPL_DEFAULTS = {
    "InputFileFilterParameterConverter": '"/test/path/file.txt"',
    "OutputFileFilterParameterConverter": '"/test/path/file.txt"',
    "BooleanFilterParameterConverter": "1",
    "LinkedBooleanFilterParameterConverter": "1",
    "InvertedBooleanFilterParameterConverter": "1",
    "DoubleFilterParameterConverter": "2.5",
    "StringFilterParameterConverter": '"TestName"',
    "LinkedPathCreationFilterParameterConverter": '"TestName"',
    "DataContainerSelectionFilterParameterConverter": '{"Data Container Name": "DataContainer"}',
    "DataContainerCreationFilterParameterConverter": '{"Data Container Name": "DataContainer"}',
    "DCPathBuilderFilterParameterConverter": '{"Data Container Name": "DataContainer"}',
    "DataArraySelectionFilterParameterConverter": '{"Data Container Name": "DataContainer", "Attribute Matrix Name": "CellData", "Data Array Name": "TestArray"}',
    "DataArraySelectionToGeometrySelectionFilterParameterConverter": '{"Data Container Name": "DataContainer", "Attribute Matrix Name": "CellData", "Data Array Name": "TestArray"}',
    "DataArrayCreationToDataObjectNameFilterParameterConverter": '{"Data Container Name": "DataContainer", "Attribute Matrix Name": "CellData", "Data Array Name": "TestArray"}',
    "AttributeMatrixSelectionFilterParameterConverter": '{"Data Container Name": "DataContainer", "Attribute Matrix Name": "CellData"}',
    "AttributeMatrixCreationFilterParameterConverter": '{"Data Container Name": "DataContainer", "Attribute Matrix Name": "CellData"}',
    "ChoiceFilterParameterConverter": "0",
    "LinkedChoicesFilterParameterConverter": "0",
    "NumericTypeFilterParameterConverter": "4",
    "ScalarTypeParameterConverter": "4",
    "FloatVec3FilterParameterConverter": '{"x": 1.0, "y": 2.0, "z": 3.0}',
    "DoubleVec3FilterParameterConverter": '{"x": 1.0, "y": 2.0, "z": 3.0}',
    "IntVec3FilterParameterConverter": '{"x": 1, "y": 2, "z": 3}',
    "UInt32Vec3FilterParameterConverter": '{"x": 1, "y": 2, "z": 3}',
    "FloatVec2FilterParameterConverter": '{"x": 1.0, "y": 2.0}',
    "AxisAngleFilterParameterConverter": '{"angle": 90.0, "h": 1.0, "k": 0.0, "l": 0.0}',
    "DynamicTableFilterParameterConverter": "[[1.0, 2.0], [3.0, 4.0]]",
    "MultiDataArraySelectionFilterParameterConverter": '[{"Data Container Name": "DC", "Attribute Matrix Name": "AM", "Data Array Name": "DA1"}, {"Data Container Name": "DC", "Attribute Matrix Name": "AM", "Data Array Name": "DA2"}]',
    "FileListInfoFilterParameterConverter": '{"EndIndex": 10, "FileExtension": ".tif", "FilePrefix": "img_", "FileSuffix": "", "IncrementIndex": 1, "InputPath": "/test/path", "Ordering": 0, "PaddingDigits": 3, "StartIndex": 0}',
    "CalculatorFilterParameterConverter": '{"InfixEquation": "1+2", "SelectedAttributeMatrix": {"Data Container Name": "DC", "Attribute Matrix Name": "AM"}}',
    "GenerateColorTableFilterParameterConverter": '"Rainbow Desaturated"',
    "RangeFilterParameterConverter": '"2.5,7.5"',
}

# Template-parameterized converters - we match these by prefix
TEMPLATE_CONVERTER_DEFAULTS = {
    "IntFilterParameterConverter": "5",
    "StringToIntFilterParameterConverter": '"5"',
    "FloatFilterParameterConverter": "2.5",
}

# Multi-parameter converters (Convert2/3Parameters) - these use multiple SIMPL keys
# The values for each SIMPL key need to be simple scalars
MULTI_PARAM_SIMPL_DEFAULTS = {
    "UInt64ToVec3FilterParameterConverter": "1",
    "FloatToVec3FilterParameterConverter": "1.0",
    "DoubleToVec3FilterParameterConverter": "1.0",
    "FloatVec3p1FilterParameterConverter": '{"x": 1.0, "y": 0.0, "z": 0.0}',
    "AMPathBuilderFilterParameterConverter": None,  # first key: DataArraySelection-like, second: string
}


def get_default_for_converter(converter_type: str) -> Optional[str]:
    """Get the default JSON value string for a converter type."""
    # Direct match
    if converter_type in SIMPL_DEFAULTS:
        return SIMPL_DEFAULTS[converter_type]

    # Template match (e.g., IntFilterParameterConverter<int32>)
    for prefix, default in TEMPLATE_CONVERTER_DEFAULTS.items():
        if converter_type.startswith(prefix):
            return default

    return None


def is_skip_converter(converter_type: str) -> bool:
    """Check if this converter type should be skipped."""
    for skip in SKIP_CONVERTERS:
        if converter_type.startswith(skip):
            return True
    return False


def parse_simpl_namespace(cpp_content: str):
    """Parse the SIMPL namespace block to extract key constants."""
    # Find the namespace SIMPL block
    pattern = r'namespace\s+SIMPL\s*\{(.*?)\}\s*//\s*namespace\s+SIMPL'
    match = re.search(pattern, cpp_content, re.DOTALL)
    if not match:
        return {}

    block = match.group(1)
    keys = {}
    # Match constexpr StringLiteral k_SomeKey = "SomeValue";
    key_pattern = r'constexpr\s+StringLiteral\s+(k_\w+)\s*=\s*"([^"]+)"'
    for m in re.finditer(key_pattern, block):
        keys[m.group(1)] = m.group(2)

    return keys


def parse_from_simpl_json(cpp_content: str, filter_name: str):
    """Parse the FromSIMPLJson method to extract conversion parameter calls."""
    # Find the FromSIMPLJson method body
    pattern = rf'Result<Arguments>\s+{re.escape(filter_name)}::FromSIMPLJson\s*\(const\s+nlohmann::json&\s+json\)\s*\{{(.*?)return\s+ConvertResultTo<Arguments>'
    match = re.search(pattern, cpp_content, re.DOTALL)
    if not match:
        # Try alternative ending pattern
        pattern2 = rf'Result<Arguments>\s+{re.escape(filter_name)}::FromSIMPLJson\s*\(const\s+nlohmann::json&\s+json\)\s*\{{(.*?)\}}\s*\}}\s*//\s*namespace'
        match = re.search(pattern2, cpp_content, re.DOTALL)
        if not match:
            return []

    body = match.group(1)

    conversions = []

    # Parse single ConvertParameter calls
    # Pattern: ConvertParameter<SIMPLConversion::ConverterType>(args, json, SIMPL::k_Key, NXKey)
    single_pattern = r'(?://\s*)?results\.push_back\s*\(\s*SIMPLConversion::ConvertParameter\s*<\s*SIMPLConversion::(\w+(?:<[^>]+>)?)\s*>\s*\(\s*args\s*,\s*json\s*,\s*SIMPL::(k_\w+)\s*,\s*([\w:]+)\s*\)\s*\)'

    # Parse Convert2Parameters calls
    convert2_pattern = r'(?://\s*)?results\.push_back\s*\(\s*SIMPLConversion::Convert2Parameters\s*<\s*SIMPLConversion::(\w+(?:<[^>]+>)?)\s*>\s*\(\s*args\s*,\s*json\s*,\s*SIMPL::(k_\w+)\s*,\s*SIMPL::(k_\w+)\s*,\s*\n?\s*([\w:]+)\s*\)\s*\)'

    # Parse Convert3Parameters calls
    convert3_pattern = r'(?://\s*)?results\.push_back\s*\(\s*SIMPLConversion::Convert3Parameters\s*<\s*SIMPLConversion::(\w+(?:<[^>]+>)?)\s*>\s*\(\s*args\s*,\s*json\s*,\s*SIMPL::(k_\w+)\s*,\s*SIMPL::(k_\w+)\s*,\s*SIMPL::(k_\w+)\s*,\s*([\w:]+)\s*\)\s*\)'

    # Track whether lines are commented out
    lines = body.split('\n')
    for line in lines:
        stripped = line.strip()
        is_commented = stripped.startswith('//')

        # Skip commented-out lines
        if is_commented:
            continue

    # Now parse from the full body, but only non-commented lines
    # First, remove all fully commented lines
    clean_lines = []
    for line in body.split('\n'):
        stripped = line.strip()
        if stripped.startswith('//'):
            continue
        clean_lines.append(line)
    clean_body = '\n'.join(clean_lines)

    # Check for conditional patterns
    # Detect if there's an if(result.valid()) or if(scalarResult.valid()) type conditional
    conditional_pattern = r'Result<>\s+(\w+)\s*=\s*SIMPLConversion::ConvertParameter\s*<\s*SIMPLConversion::(\w+(?:<[^>]+>)?)\s*>\s*\(\s*args\s*,\s*json\s*,\s*SIMPL::(k_\w+)\s*,\s*([\w:]+)\s*\)\s*;\s*\n\s*if\s*\(\s*\1\.valid\(\)\s*\)'
    conditional_matches = set()
    for m in re.finditer(conditional_pattern, clean_body, re.DOTALL):
        conditional_matches.add(m.group(3))  # SIMPL key
        conversions.append({
            'type': 'single',
            'converter': m.group(2),
            'simpl_keys': [m.group(3)],
            'nx_key': m.group(4),
            'conditional': True
        })

    # Parse Convert3Parameters first (more specific)
    for m in re.finditer(convert3_pattern, clean_body):
        conversions.append({
            'type': 'triple',
            'converter': m.group(1),
            'simpl_keys': [m.group(2), m.group(3), m.group(4)],
            'nx_key': m.group(5),
            'conditional': False
        })

    # Parse Convert2Parameters
    for m in re.finditer(convert2_pattern, clean_body):
        conversions.append({
            'type': 'double',
            'converter': m.group(1),
            'simpl_keys': [m.group(2), m.group(3)],
            'nx_key': m.group(4),
            'conditional': False
        })

    # Parse single ConvertParameter (but skip ones already captured as conditional)
    for m in re.finditer(single_pattern, clean_body):
        simpl_key = m.group(2)
        if simpl_key in conditional_matches:
            continue
        nx_key = m.group(3)
        # Skip placeholder parameters
        if '@SIMPLNX_PARAMETER_KEY@' in nx_key:
            continue
        conversions.append({
            'type': 'single',
            'converter': m.group(1),
            'simpl_keys': [simpl_key],
            'nx_key': nx_key,
            'conditional': False
        })

    return conversions


def parse_legacy_uuid_mapping(mapping_file: Path):
    """Parse a LegacyUUIDMapping.hpp file to get SIMPL UUID -> FilterName mapping."""
    content = mapping_file.read_text()
    result = {}

    # Pattern: {Uuid::FromString("uuid").value(), {FilterTraits<FilterName>::uuid, &FilterName::FromSIMPLJson}}
    pattern = r'Uuid::FromString\("([^"]+)"\)\.value\(\)\s*,\s*\{\s*nx::core::FilterTraits<(\w+)>::uuid\s*,\s*&\2::FromSIMPLJson\s*\}'
    for m in re.finditer(pattern, content):
        uuid = m.group(1)
        filter_class = m.group(2)
        # Store the first UUID found for each filter (avoid duplicates from commented-out entries)
        if filter_class not in result:
            result[filter_class] = uuid
        # But also handle multiple UUIDs mapping to same filter
        # We want to track ALL UUIDs that map to a filter
    return result


def parse_legacy_filter_names(filepath: Path):
    """Parse LegacySimplFilterUuid.hpp to get FilterName -> UUID mapping."""
    content = filepath.read_text()
    result = {}
    # Pattern: {"FilterName", "uuid-string"},
    pattern = r'\{"(\w+)"\s*,\s*"([^"]+)"\}'
    for m in re.finditer(pattern, content):
        name = m.group(1)
        uuid = m.group(2)
        result[name] = uuid
        # Also build reverse: UUID -> name(s)
    # Build reverse map
    uuid_to_names = {}
    for name, uuid in result.items():
        if uuid not in uuid_to_names:
            uuid_to_names[uuid] = []
        uuid_to_names[uuid].append(name)
    return result, uuid_to_names


def find_test_file(plugin_dir: Path, filter_name: str):
    """Find the test .cpp file for a given filter."""
    test_dir = plugin_dir / "test"
    if not test_dir.exists():
        return None

    # Remove "Filter" suffix for test file lookup
    base_name = filter_name
    if base_name.endswith("Filter"):
        base_name = base_name[:-6]

    # Try various naming patterns
    candidates = [
        test_dir / f"{filter_name}Test.cpp",       # FilterNameFilterTest.cpp
        test_dir / f"{base_name}Test.cpp",          # FilterNameTest.cpp
        test_dir / f"{base_name}FilterTest.cpp",    # FilterNameFilterTest.cpp (explicit)
    ]

    for candidate in candidates:
        if candidate.exists():
            return candidate

    return None


def get_plugin_name(plugin_dir: Path) -> str:
    """Get the plugin name from the directory structure."""
    return plugin_dir.name


def get_filter_human_label(filter_name: str) -> str:
    """Generate a reasonable human label from filter name."""
    # Remove "Filter" suffix
    name = filter_name
    if name.endswith("Filter"):
        name = name[:-6]
    # Add spaces before capitals
    result = re.sub(r'([a-z])([A-Z])', r'\1 \2', name)
    result = re.sub(r'([A-Z]+)([A-Z][a-z])', r'\1 \2', result)
    return result


def build_simpl_json_value(converter_type: str, simpl_key_name: str):
    """Build the JSON value for a SIMPL parameter based on converter type."""
    default = get_default_for_converter(converter_type)
    if default is None:
        return None
    return json.loads(default)


def _converter_priority(converter: str) -> int:
    """Return a priority for converter types. Higher = more detailed JSON value.

    When the same SIMPL key is used by multiple converters, we want to write
    the most detailed JSON format so all converters can parse their data from it.
    E.g., DataArraySelection (3 fields) > AttributeMatrixSelection (2 fields) > DataContainerSelection (1 field).
    """
    if converter == "DataArraySelectionFilterParameterConverter":
        return 30
    if converter == "DataArraySelectionToGeometrySelectionFilterParameterConverter":
        return 30
    if converter == "DataArrayCreationToDataObjectNameFilterParameterConverter":
        return 30
    if converter in ("AttributeMatrixSelectionFilterParameterConverter", "AttributeMatrixCreationFilterParameterConverter"):
        return 20
    if converter in ("DataContainerSelectionFilterParameterConverter", "DataContainerCreationFilterParameterConverter", "DCPathBuilderFilterParameterConverter"):
        return 10
    return 0


def generate_fixture_json(filter_name: str, simpl_uuid: str, filter_legacy_name: str,
                          simpl_keys: dict, conversions: list, is_65: bool):
    """Generate a SIMPL pipeline JSON fixture."""
    human_label = get_filter_human_label(filter_name)
    version_label = "Backwards Compatibility Test" if is_65 else "6.4 Backwards Compatibility Test"

    fixture = {
        "PipelineBuilder": {
            "Name": f"{get_filter_human_label(filter_name)} {version_label}",
            "Number_Filters": 1,
            "Version": 6
        }
    }

    filter_json = {
        "Filter_Enabled": True,
        "Filter_Human_Label": human_label,
        "Filter_Name": filter_legacy_name,
    }

    if is_65:
        filter_json["Filter_Uuid"] = "{" + simpl_uuid + "}"

    # First pass: for each SIMPL key name, find the best (most detailed) converter
    # so we can write the right JSON format when a key is shared
    best_converter_for_key = {}  # simpl_key_name -> (converter, priority)

    for conv in conversions:
        converter = conv['converter']
        if conv['conditional'] and not is_65:
            continue
        if is_skip_converter(converter):
            continue

        if conv['type'] == 'single':
            simpl_key_var = conv['simpl_keys'][0]
            if simpl_key_var not in simpl_keys:
                continue
            simpl_key_name = simpl_keys[simpl_key_var]
            priority = _converter_priority(converter)
            if simpl_key_name not in best_converter_for_key or priority > best_converter_for_key[simpl_key_name][1]:
                best_converter_for_key[simpl_key_name] = (converter, priority)

    # Second pass: write the JSON values
    added_simpl_keys = set()

    for conv in conversions:
        converter = conv['converter']

        # Skip conditional parameters in 6_4 fixture
        if conv['conditional'] and not is_65:
            continue

        if is_skip_converter(converter):
            continue

        if conv['type'] == 'single':
            simpl_key_var = conv['simpl_keys'][0]
            if simpl_key_var not in simpl_keys:
                logger.debug(f"  SIMPL key variable {simpl_key_var} not found in namespace for {filter_name}")
                continue
            simpl_key_name = simpl_keys[simpl_key_var]

            if simpl_key_name in added_simpl_keys:
                continue

            # Use the best converter for this key
            best_converter = best_converter_for_key.get(simpl_key_name, (converter, 0))[0]
            value = build_simpl_json_value(best_converter, simpl_key_name)
            if value is not None:
                filter_json[simpl_key_name] = value
                added_simpl_keys.add(simpl_key_name)

        elif conv['type'] == 'double':
            for key_var in conv['simpl_keys']:
                if key_var not in simpl_keys:
                    logger.debug(f"  SIMPL key variable {key_var} not found in namespace for {filter_name}")
                    continue
                simpl_key_name = simpl_keys[key_var]

                if simpl_key_name in added_simpl_keys:
                    continue

                converter_base = conv['converter']
                # For Convert2Parameters, each SIMPL key gets its own value
                if converter_base in ("AMPathBuilderFilterParameterConverter",):
                    # First key is DataArraySelection-like, second is a string
                    idx = conv['simpl_keys'].index(key_var)
                    if idx == 0:
                        # Check if this key is also used by a single converter
                        if simpl_key_name in best_converter_for_key:
                            best_converter = best_converter_for_key[simpl_key_name][0]
                            value = build_simpl_json_value(best_converter, simpl_key_name)
                        else:
                            value = json.loads('{"Data Container Name": "DataContainer", "Attribute Matrix Name": "CellData", "Data Array Name": "TestArray"}')
                    else:
                        value = "TestName"
                elif converter_base in ("FloatVec3p1FilterParameterConverter",):
                    # First key is a float vec3, second is a float (angle)
                    idx = conv['simpl_keys'].index(key_var)
                    if idx == 0:
                        value = json.loads('{"x": 1.0, "y": 0.0, "z": 0.0}')
                    else:
                        value = 90.0
                else:
                    # Generic multi-param: each key gets a scalar
                    for prefix, default in MULTI_PARAM_SIMPL_DEFAULTS.items():
                        if converter_base.startswith(prefix) and default is not None:
                            value = json.loads(default)
                            break
                    else:
                        # Try int/float scalar defaults
                        if "UInt64" in converter_base or "Int" in converter_base:
                            value = 1
                        elif "Float" in converter_base or "Double" in converter_base:
                            value = 1.0
                        else:
                            value = 1

                filter_json[simpl_key_name] = value
                added_simpl_keys.add(simpl_key_name)

        elif conv['type'] == 'triple':
            for key_var in conv['simpl_keys']:
                if key_var not in simpl_keys:
                    logger.debug(f"  SIMPL key variable {key_var} not found in namespace for {filter_name}")
                    continue
                simpl_key_name = simpl_keys[key_var]

                if simpl_key_name in added_simpl_keys:
                    continue

                converter_base = conv['converter']
                if "UInt64" in converter_base or "Int" in converter_base:
                    value = 1
                elif "Float" in converter_base or "Double" in converter_base:
                    value = 1.0
                else:
                    value = 1

                filter_json[simpl_key_name] = value
                added_simpl_keys.add(simpl_key_name)

    fixture["0"] = filter_json
    return fixture


def get_check_line(filter_name: str, converter: str, nx_key: str, conditional: bool):
    """Generate a CHECK line for the test based on converter type."""
    # Clean the NX key - it might be fully qualified like FilterName::k_Key
    # or just k_Key
    if '::' not in nx_key:
        nx_key_full = f"{filter_name}::{nx_key}"
    else:
        nx_key_full = nx_key

    # Determine the check based on converter type
    check = None

    if converter in ("InputFileFilterParameterConverter", "OutputFileFilterParameterConverter"):
        check = f'CHECK(args.value<FileSystemPathParameter::ValueType>({nx_key_full}) == fs::path("/test/path/file.txt"));'
    elif converter in ("BooleanFilterParameterConverter", "LinkedBooleanFilterParameterConverter"):
        check = f'CHECK(args.value<bool>({nx_key_full}) == true);'
    elif converter == "InvertedBooleanFilterParameterConverter":
        check = f'CHECK(args.value<bool>({nx_key_full}) == false);'
    elif converter == "DoubleFilterParameterConverter":
        check = f'CHECK(args.value<float64>({nx_key_full}) == 2.5);'
    elif converter.startswith("FloatFilterParameterConverter"):
        check = f'CHECK(args.value<float32>({nx_key_full}) == 2.5f);'
    elif converter in ("StringFilterParameterConverter", "LinkedPathCreationFilterParameterConverter"):
        check = f'CHECK(args.value<std::string>({nx_key_full}) == "TestName");'
    elif converter in ("DataContainerSelectionFilterParameterConverter", "DCPathBuilderFilterParameterConverter"):
        check = f'CHECK(args.value<DataPath>({nx_key_full}) == DataPath({{"DataContainer"}}));'
    elif converter == "DataContainerCreationFilterParameterConverter":
        check = f'CHECK(args.value<DataPath>({nx_key_full}) == DataPath({{"DataContainer"}}));'
    elif converter == "DataArraySelectionFilterParameterConverter":
        check = f'CHECK(args.value<DataPath>({nx_key_full}) == DataPath({{"DataContainer", "CellData", "TestArray"}}));'
    elif converter == "DataArraySelectionToGeometrySelectionFilterParameterConverter":
        check = f'CHECK(args.value<DataPath>({nx_key_full}) == DataPath({{"DataContainer"}}));'
    elif converter == "DataArrayCreationToDataObjectNameFilterParameterConverter":
        check = f'CHECK(args.value<std::string>({nx_key_full}) == "TestArray");'
    elif converter in ("AttributeMatrixSelectionFilterParameterConverter", "AttributeMatrixCreationFilterParameterConverter"):
        check = f'CHECK(args.value<DataPath>({nx_key_full}) == DataPath({{"DataContainer", "CellData"}}));'
    elif converter in ("ChoiceFilterParameterConverter", "LinkedChoicesFilterParameterConverter"):
        check = f'CHECK(args.value<ChoicesParameter::ValueType>({nx_key_full}) == 0);'
    elif converter in ("NumericTypeFilterParameterConverter",):
        check = f'// Complex type - verified by successful pipeline loading'
    elif converter in ("ScalarTypeParameterConverter",):
        check = f'// Complex type - verified by successful pipeline loading'
    elif converter == "GenerateColorTableFilterParameterConverter":
        check = f'CHECK(args.value<std::string>({nx_key_full}) == "Rainbow Desaturated");'
    elif converter.startswith("IntFilterParameterConverter"):
        # Extract the template type
        tmatch = re.search(r'IntFilterParameterConverter<(\w+)>', converter)
        if tmatch:
            int_type = tmatch.group(1)
            check = f'CHECK(args.value<{int_type}>({nx_key_full}) == 5);'
        else:
            check = f'CHECK(args.value<int32>({nx_key_full}) == 5);'
    elif converter.startswith("StringToIntFilterParameterConverter"):
        tmatch = re.search(r'StringToIntFilterParameterConverter<(\w+)>', converter)
        if tmatch:
            int_type = tmatch.group(1)
            check = f'CHECK(args.value<{int_type}>({nx_key_full}) == 5);'
        else:
            check = f'CHECK(args.value<int32>({nx_key_full}) == 5);'
    else:
        # Complex types - just verify by loading
        check = f'// Complex type ({converter}) - verified by successful pipeline loading'

    return check


def generate_test_code(filter_name: str, plugin_name: str, conversions: list):
    """Generate the TEST_CASE code for backwards compatibility testing."""
    # Strip 'Filter' suffix for the test tag if present
    tag_name = filter_name

    lines = []
    lines.append(f'')
    lines.append(f'TEST_CASE("{plugin_name}::{filter_name}: SIMPL Backwards Compatibility", "[{plugin_name}][{filter_name}][BackwardsCompatibility]")')
    lines.append(f'{{')
    lines.append(f'  auto app = Application::GetOrCreateInstance();')
    lines.append(f'  UnitTest::LoadPlugins();')
    lines.append(f'  auto filterList = app->getFilterList();')
    lines.append(f'')
    lines.append(f'  const fs::path conversionDir = fs::path(unit_test::k_SourceDir.view()) / "test" / "simpl_conversion";')
    lines.append(f'')
    lines.append(f'  const std::vector<std::pair<std::string, fs::path>> fixtures = {{')
    lines.append(f'      {{"SIMPL 6.5 (UUID)", conversionDir / "6_5" / "{filter_name}.json"}},')
    lines.append(f'      {{"SIMPL 6.4 (Filter_Name)", conversionDir / "6_4" / "{filter_name}.json"}},')
    lines.append(f'  }};')
    lines.append(f'')
    lines.append(f'  for(const auto& [label, fixturePath] : fixtures)')
    lines.append(f'  {{')
    lines.append(f'    DYNAMIC_SECTION(label)')
    lines.append(f'    {{')
    lines.append(f'      auto pipelineResult = Pipeline::FromSIMPLFile(fixturePath, filterList);')
    lines.append(f'      REQUIRE(pipelineResult.valid());')
    lines.append(f'')
    lines.append(f'      auto& pipeline = pipelineResult.value();')
    lines.append(f'      REQUIRE(pipeline.size() == 1);')
    lines.append(f'')
    lines.append(f'      auto* pipelineFilter = dynamic_cast<PipelineFilter*>(pipeline.at(0));')
    lines.append(f'      REQUIRE(pipelineFilter != nullptr);')
    lines.append(f'')
    lines.append(f'      const IFilter* filter = pipelineFilter->getFilter();')
    lines.append(f'      REQUIRE(filter != nullptr);')
    lines.append(f'      REQUIRE(filter->uuid() == FilterTraits<{filter_name}>::uuid);')
    lines.append(f'')
    lines.append(f'      CHECK(pipelineFilter->getComments().empty());')
    lines.append(f'')
    lines.append(f'      const Arguments args = pipelineFilter->getArguments();')

    # Generate CHECK lines
    has_conditional = any(c['conditional'] for c in conversions)
    has_65_only_block = False

    for conv in conversions:
        converter = conv['converter']
        nx_key = conv['nx_key']
        conditional = conv['conditional']

        if is_skip_converter(converter):
            continue

        # Skip placeholder keys
        if '@SIMPLNX_PARAMETER_KEY@' in nx_key:
            continue

        # Skip multi-param converters for CHECK lines (complex)
        if conv['type'] in ('double', 'triple'):
            lines.append(f'      // Complex type ({converter}) - verified by successful pipeline loading')
            continue

        check_line = get_check_line(filter_name, converter, nx_key, conditional)

        if conditional:
            if not has_65_only_block:
                lines.append(f'      if(label == "SIMPL 6.5 (UUID)")')
                lines.append(f'      {{')
                has_65_only_block = True
            lines.append(f'        {check_line}')
        else:
            if has_65_only_block:
                lines.append(f'      }}')
                has_65_only_block = False
            lines.append(f'      {check_line}')

    if has_65_only_block:
        lines.append(f'      }}')

    lines.append(f'    }}')
    lines.append(f'  }}')
    lines.append(f'}}')

    return '\n'.join(lines)


def ensure_includes(test_file: Path, plugin_name: str, filter_name: str):
    """Ensure the test file has the necessary includes for the backwards compat test."""
    content = test_file.read_text()
    original_content = content

    needed_includes = [
        '#include "simplnx/Core/Application.hpp"',
        '#include "simplnx/Pipeline/Pipeline.hpp"',
        '#include "simplnx/Pipeline/PipelineFilter.hpp"',
    ]

    needed_std_includes = [
        '#include <fstream>',
    ]

    # Check and add missing includes
    additions = []
    for inc in needed_includes:
        if inc not in content:
            additions.append(inc)

    std_additions = []
    for inc in needed_std_includes:
        if inc not in content:
            std_additions.append(inc)

    if not additions and not std_additions:
        return content

    # Find the right place to insert includes
    # Insert simplnx includes after the last existing simplnx include
    lines = content.split('\n')
    last_simplnx_include = -1
    last_std_include = -1
    for i, line in enumerate(lines):
        if line.strip().startswith('#include "simplnx/'):
            last_simplnx_include = i
        if line.strip().startswith('#include <'):
            last_std_include = i

    if additions:
        if last_simplnx_include >= 0:
            for inc in reversed(additions):
                lines.insert(last_simplnx_include + 1, inc)
                # Adjust indices
                if last_std_include > last_simplnx_include:
                    last_std_include += 1
        else:
            # Insert after the first include block
            insert_pos = 0
            for i, line in enumerate(lines):
                if line.strip().startswith('#include'):
                    insert_pos = i + 1
            for inc in reversed(additions):
                lines.insert(insert_pos, inc)

    if std_additions:
        # Recalculate last_std_include
        last_std_include = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('#include <'):
                last_std_include = i

        if last_std_include >= 0:
            for inc in reversed(std_additions):
                lines.insert(last_std_include + 1, inc)
        else:
            # Find end of include block
            insert_pos = 0
            for i, line in enumerate(lines):
                if line.strip().startswith('#include'):
                    insert_pos = i + 1
            for inc in reversed(std_additions):
                lines.insert(insert_pos, inc)

    content = '\n'.join(lines)

    # Ensure namespace fs = std::filesystem is present
    if 'namespace fs = std::filesystem' not in content:
        # Find the "using namespace" lines and add after them
        lines = content.split('\n')
        insert_pos = None
        for i, line in enumerate(lines):
            if line.strip().startswith('using namespace'):
                insert_pos = i + 1
        if insert_pos is not None:
            lines.insert(insert_pos, 'namespace fs = std::filesystem;')
        else:
            # Find after the last #include or after <filesystem>
            for i, line in enumerate(lines):
                if '<filesystem>' in line or '<fstream>' in line:
                    insert_pos = i + 1
            if insert_pos is not None:
                lines.insert(insert_pos, '')
                lines.insert(insert_pos + 1, 'namespace fs = std::filesystem;')
        content = '\n'.join(lines)

    # Also ensure <filesystem> is included
    if '#include <filesystem>' not in content:
        lines = content.split('\n')
        last_std = -1
        for i, line in enumerate(lines):
            if line.strip().startswith('#include <'):
                last_std = i
        if last_std >= 0:
            lines.insert(last_std + 1, '#include <filesystem>')
        content = '\n'.join(lines)

    return content


def find_filter_name_for_uuid(uuid: str, uuid_to_names: dict) -> Optional[str]:
    """Find the SIMPL Filter_Name for a given UUID."""
    if uuid in uuid_to_names:
        return uuid_to_names[uuid][0]
    return None


def discover_filters():
    """Discover all filter .cpp files across all plugins."""
    plugins_dir = ROOT / "src" / "Plugins"
    filters = []

    for plugin_dir in sorted(plugins_dir.iterdir()):
        if not plugin_dir.is_dir():
            continue

        plugin_name = plugin_dir.name
        # Find filter .cpp files
        filter_dir = plugin_dir / "src" / plugin_name / "Filters"
        if not filter_dir.exists():
            continue

        for cpp_file in sorted(filter_dir.glob("*.cpp")):
            filter_name = cpp_file.stem
            # Only process filters that have FromSIMPLJson
            content = cpp_file.read_text()
            if 'FromSIMPLJson' not in content:
                continue
            if 'namespace SIMPL' not in content:
                continue

            filters.append({
                'plugin_dir': plugin_dir,
                'plugin_name': plugin_name,
                'filter_name': filter_name,
                'cpp_file': cpp_file,
                'content': content,
            })

    return filters


def main():
    logger.info("Starting backwards compatibility test generation")

    # Parse legacy UUID mappings
    logger.info("Parsing legacy UUID mappings...")

    legacy_mappings = {}  # filter_class -> simpl_uuid
    for mapping_file in (ROOT / "src" / "Plugins").rglob("*LegacyUUIDMapping.hpp"):
        mapping = parse_legacy_uuid_mapping(mapping_file)
        legacy_mappings.update(mapping)
        logger.info(f"  Parsed {len(mapping)} mappings from {mapping_file.name}")

    # Parse Filter_Name -> UUID mapping
    logger.info("Parsing legacy filter names...")
    legacy_names_file = ROOT / "src" / "simplnx" / "Pipeline" / "LegacySimplFilterUuid.hpp"
    filter_name_to_uuid, uuid_to_filter_names = parse_legacy_filter_names(legacy_names_file)
    logger.info(f"  Found {len(filter_name_to_uuid)} legacy filter names")

    # Discover all filters
    logger.info("Discovering filters...")
    filters = discover_filters()
    logger.info(f"  Found {len(filters)} filters with FromSIMPLJson")

    # Statistics
    generated = 0
    skipped = 0
    errors = 0

    for finfo in filters:
        filter_name = finfo['filter_name']
        plugin_name = finfo['plugin_name']
        plugin_dir = finfo['plugin_dir']
        cpp_file = finfo['cpp_file']
        content = finfo['content']

        # Skip filters that already have tests
        if filter_name in SKIP_FILTERS:
            logger.info(f"  SKIP (already has test): {filter_name}")
            skipped += 1
            continue

        # Parse SIMPL namespace
        simpl_keys = parse_simpl_namespace(content)
        if not simpl_keys:
            logger.warning(f"  SKIP (no SIMPL keys): {filter_name}")
            skipped += 1
            continue

        # Parse FromSIMPLJson conversions
        conversions = parse_from_simpl_json(content, filter_name)
        if not conversions:
            logger.warning(f"  SKIP (no conversions found): {filter_name}")
            skipped += 1
            continue

        # Look up SIMPL UUID
        simpl_uuid = legacy_mappings.get(filter_name)
        if not simpl_uuid:
            logger.warning(f"  SKIP (no SIMPL UUID in legacy mapping): {filter_name}")
            skipped += 1
            continue

        # Look up Filter_Name for 6.4 path
        filter_legacy_name = find_filter_name_for_uuid(simpl_uuid, uuid_to_filter_names)
        if not filter_legacy_name:
            logger.warning(f"  SKIP (no Filter_Name for UUID {simpl_uuid}): {filter_name}")
            skipped += 1
            continue

        # Find test file
        test_file = find_test_file(plugin_dir, filter_name)
        if not test_file:
            logger.warning(f"  SKIP (no test file found): {filter_name}")
            skipped += 1
            continue

        # Check if test file already has backwards compat test
        test_content = test_file.read_text()
        if "BackwardsCompatibility" in test_content:
            logger.info(f"  SKIP (test file already has BackwardsCompatibility): {filter_name}")
            skipped += 1
            continue

        logger.info(f"  Generating for {filter_name} ({plugin_name})")
        logger.info(f"    SIMPL UUID: {simpl_uuid}")
        logger.info(f"    Filter_Name: {filter_legacy_name}")
        logger.info(f"    Conversions: {len(conversions)}")
        logger.info(f"    Test file: {test_file}")

        # Generate JSON fixtures
        fixture_65 = generate_fixture_json(
            filter_name, simpl_uuid, filter_legacy_name,
            simpl_keys, conversions, is_65=True
        )
        fixture_64 = generate_fixture_json(
            filter_name, simpl_uuid, filter_legacy_name,
            simpl_keys, conversions, is_65=False
        )

        # Write fixture files
        fixture_dir_65 = plugin_dir / "test" / "simpl_conversion" / "6_5"
        fixture_dir_64 = plugin_dir / "test" / "simpl_conversion" / "6_4"
        fixture_dir_65.mkdir(parents=True, exist_ok=True)
        fixture_dir_64.mkdir(parents=True, exist_ok=True)

        fixture_path_65 = fixture_dir_65 / f"{filter_name}.json"
        fixture_path_64 = fixture_dir_64 / f"{filter_name}.json"

        with open(fixture_path_65, 'w') as f:
            json.dump(fixture_65, f, indent=2)
            f.write('\n')

        with open(fixture_path_64, 'w') as f:
            json.dump(fixture_64, f, indent=2)
            f.write('\n')

        logger.info(f"    Wrote {fixture_path_65}")
        logger.info(f"    Wrote {fixture_path_64}")

        # Generate test code
        test_code = generate_test_code(filter_name, plugin_name, conversions)

        # Ensure includes are present
        updated_test_content = ensure_includes(test_file, plugin_name, filter_name)

        # Append test code to test file
        # Make sure there's a newline at the end before appending
        if not updated_test_content.endswith('\n'):
            updated_test_content += '\n'

        updated_test_content += test_code + '\n'

        test_file.write_text(updated_test_content)
        logger.info(f"    Updated test file: {test_file}")

        generated += 1

    logger.info(f"\nSummary:")
    logger.info(f"  Generated: {generated}")
    logger.info(f"  Skipped: {skipped}")
    logger.info(f"  Errors: {errors}")
    logger.info(f"  Total filters found: {len(filters)}")


if __name__ == "__main__":
    main()
