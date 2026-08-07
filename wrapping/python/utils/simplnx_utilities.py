"""Simplnx utilities for converting live Pipeline objects to runnable Python scripts.

Usage:
    import simplnx as nx
    from simplnx_utilities import generate_python_pipeline, generate_python_filters

    pipeline = nx.Pipeline.from_file("path/to/pipeline.d3dpipeline")
    print(generate_python_pipeline(pipeline))
"""

import pathlib
from dataclasses import dataclass, field
from typing import Any

import simplnx as nx
import orientationanalysis as nxor
import itkimageprocessing as nxitk
# Ensure filters are loaded even though the modules aren't used directly
assert nxitk
assert nxor

# ---------------------------------------------------------------------------
# Module alias map
# ---------------------------------------------------------------------------

MODULE_ALIASES: dict[str, str] = {
    "simplnx" : "nx",
    "orientationanalysis" : "nxor",
    "itkimageprocessing" : "nxitk",
}

# Canonical import ordering for generated scripts
_MODULE_ORDER: tuple[str, ...] = ("simplnx", "orientationanalysis", "itkimageprocessing")


# ---------------------------------------------------------------------------
# CodeGenContext
# ---------------------------------------------------------------------------

@dataclass
class CodeGenContext:
    """State carried through code generation for variable naming."""

    filter_index: int = 0
    variable_counter: dict[str, int] = field(default_factory=dict)

    def unique_name(self, prefix: str) -> str:
        """Return a unique variable name like 'threshold_1', 'threshold_2'."""
        count = self.variable_counter.get(prefix, 0) + 1
        self.variable_counter[prefix] = count
        return f"{prefix}_{count}"


# ---------------------------------------------------------------------------
# Enum and DataPath helpers
# ---------------------------------------------------------------------------

def _is_pybind11_enum(value: Any) -> bool:
    """Check if a value is a pybind11-bound enum."""
    return hasattr(type(value), "__members__") and hasattr(value, "name") and hasattr(value, "value")


def _encode_enum(value: Any) -> str:
    """Produce e.g. 'nx.DataType.boolean' or 'nx.ArrayThreshold.ComparisonType.GreaterThan'."""
    t = type(value)
    module_name = t.__module__
    alias = MODULE_ALIASES.get(module_name, module_name)
    qualname = t.__qualname__
    return f"{alias}.{qualname}.{value.name}"


def _is_datapath(value: Any) -> bool:
    """Check if a value is a simplnx DataPath."""
    return isinstance(value, nx.DataPath)


def _encode_datapath(value: nx.DataPath) -> str:
    """Produce e.g. 'nx.DataPath("DataContainer/Cell Data")'."""
    path_str = value.to_string("/")
    return f"nx.DataPath({repr(path_str)})"


def _encode_simple_value(value: Any) -> str:
    """Encode a single simple value to a Python expression string."""
    if _is_pybind11_enum(value):
        return _encode_enum(value)
    if _is_datapath(value):
        return _encode_datapath(value)
    if isinstance(value, pathlib.PurePath):
        return repr(str(value))
    if isinstance(value, bool):
        return repr(value)
    if isinstance(value, float):
        return _encode_float(value)
    if isinstance(value, int):
        return repr(value)
    if isinstance(value, str):
        return repr(value)
    if isinstance(value, list):
        return _encode_list(value)
    return repr(value)


def _encode_float(value: float) -> str:
    """Encode a float, rounding away float32 precision artifacts."""
    for digits in (6, 8, 10):
        rounded = round(value, digits)
        if abs(rounded - value) < abs(value) * 1e-7 or abs(rounded - value) < 1e-12:
            s = f"{rounded:.{digits}f}".rstrip("0").rstrip(".")
            if "." not in s:
                s += ".0"
            return s
    return repr(value)


def _encode_list(values: list) -> str:
    """Encode a list, handling DataPath and enum elements."""
    if not values:
        return "[]"
    items = [_encode_simple_value(v) for v in values]
    joined = ", ".join(items)
    if len(joined) > 80:
        inner = ",\n    ".join(items)
        return f"[\n    {inner}\n]"
    return f"[{joined}]"


# ---------------------------------------------------------------------------
# Codec functions — each encodes a specific parameter value type
# ---------------------------------------------------------------------------

def _encode_default(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Fallback encoder using repr() with special handling for DataPath and enums."""
    return [_encode_simple_value(value)]


def _encode_array_threshold_set(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode ArrayThresholdSet construction."""
    lines: list[str] = []
    threshold_vars: list[str] = []

    for threshold in value.thresholds:
        var = context.unique_name("threshold")
        threshold_vars.append(var)
        lines.append(f"{var} = nx.ArrayThreshold()")
        lines.append(f"{var}.array_path = {_encode_datapath(threshold.array_path)}")
        lines.append(f"{var}.comparison = {_encode_enum(threshold.comparison)}")
        lines.append(f"{var}.value = {repr(threshold.value)}")
        if threshold.component_index != 0:
            lines.append(f"{var}.component_index = {threshold.component_index}")
        if threshold.inverted:
            lines.append(f"{var}.inverted = True")
        if threshold.union_op.name != "And":
            lines.append(f"{var}.union_op = {_encode_enum(threshold.union_op)}")

    set_var = context.unique_name("threshold_set")
    lines.append(f"{set_var} = nx.ArrayThresholdSet()")
    lines.append(f"{set_var}.thresholds = [{', '.join(threshold_vars)}]")
    if value.inverted:
        lines.append(f"{set_var}.inverted = True")
    if value.union_op.name != "And":
        lines.append(f"{set_var}.union_op = {_encode_enum(value.union_op)}")

    lines.append(set_var)  # expression line
    return lines


def _encode_generated_file_list(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode GeneratedFileListParameter.ValueType construction."""
    var = context.unique_name("file_list")
    lines = [f"{var} = nx.GeneratedFileListParameter.ValueType()"]
    lines.append(f"{var}.input_path = {repr(value.input_path)}")
    lines.append(f"{var}.ordering = {_encode_enum(value.ordering)}")
    lines.append(f"{var}.file_prefix = {repr(value.file_prefix)}")
    lines.append(f"{var}.file_suffix = {repr(value.file_suffix)}")
    lines.append(f"{var}.file_extension = {repr(value.file_extension)}")
    lines.append(f"{var}.start_index = {repr(value.start_index)}")
    lines.append(f"{var}.end_index = {repr(value.end_index)}")
    lines.append(f"{var}.increment_index = {repr(value.increment_index)}")
    lines.append(f"{var}.padding_digits = {repr(value.padding_digits)}")
    lines.append(var)
    return lines


def _encode_read_h5ebsd(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode ReadH5EbsdFileParameter.ValueType construction."""
    var = context.unique_name("h5ebsd_param")
    lines = [f"{var} = nxor.ReadH5EbsdFileParameter.ValueType()"]
    lines.append(f"{var}.input_file_path = {repr(value.input_file_path)}")
    lines.append(f"{var}.start_slice = {repr(value.start_slice)}")
    lines.append(f"{var}.end_slice = {repr(value.end_slice)}")
    lines.append(f"{var}.euler_representation = {repr(value.euler_representation)}")
    lines.append(f"{var}.selected_array_names = {repr(value.selected_array_names)}")
    lines.append(f"{var}.use_recommended_transform = {repr(value.use_recommended_transform)}")
    lines.append(var)
    return lines


def _encode_calculator(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode CalculatorParameter.ValueType construction."""
    var = context.unique_name("calc_param")
    selected_group = _encode_datapath(value.selected_group) if _is_datapath(value.selected_group) else repr(value.selected_group)
    units = _encode_enum(value.units) if _is_pybind11_enum(value.units) else repr(value.units)
    lines = [f"{var} = nx.CalculatorParameter.ValueType({selected_group}, {repr(value.equation)}, {units})"]
    lines.append(var)
    return lines


def _encode_dream3d_import(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode Dream3dImportParameter.ImportData construction."""
    var = context.unique_name("import_data")
    lines = [f"{var} = nx.Dream3dImportParameter.ImportData()"]
    lines.append(f"{var}.file_path = {repr(str(value.file_path))}")
    if hasattr(value, "data_paths") and value.data_paths:
        paths_str = _encode_list([dp for dp in value.data_paths])
        lines.append(f"{var}.data_paths = {paths_str}")
    if hasattr(value, "import_policy") and _is_pybind11_enum(value.import_policy):
        lines.append(f"{var}.import_policy = {_encode_enum(value.import_policy)}")
    lines.append(var)
    return lines


def _encode_read_hdf5_dataset(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode ReadHDF5DatasetParameter.ValueType construction."""
    var = context.unique_name("hdf5_param")
    lines = [f"{var} = nx.ReadHDF5DatasetParameter.ValueType()"]
    lines.append(f"{var}.input_file = {repr(value.input_file)}")
    if hasattr(value, "parent") and value.parent is not None and _is_datapath(value.parent):
        lines.append(f"{var}.parent = {_encode_datapath(value.parent)}")
    if hasattr(value, "datasets"):
        dataset_lines = []
        for ds in value.datasets:
            ds_var = context.unique_name("dataset_info")
            lines.append(f"{ds_var} = nx.DatasetImportInfo()")
            lines.append(f"{ds_var}.data_set_path = {repr(ds.data_set_path)}")
            lines.append(f"{ds_var}.component_dimensions = {repr(ds.component_dimensions)}")
            lines.append(f"{ds_var}.tuple_dimensions = {repr(ds.tuple_dimensions)}")
            dataset_lines.append(ds_var)
        lines.append(f"{var}.datasets = [{', '.join(dataset_lines)}]")
    lines.append(var)
    return lines


def _encode_read_csv_data(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode ReadCSVData construction."""
    var = context.unique_name("csv_data")
    lines = [f"{var} = nx.ReadCSVData()"]
    lines.append(f"{var}.input_file_path = {repr(value.input_file_path)}")
    lines.append(f"{var}.start_import_row = {repr(value.start_import_row)}")
    lines.append(f"{var}.delimiters = {repr(value.delimiters)}")
    lines.append(f"{var}.consecutive_delimiters = {repr(value.consecutive_delimiters)}")
    if hasattr(value, "custom_headers"):
        lines.append(f"{var}.custom_headers = {repr(value.custom_headers)}")
    if hasattr(value, "data_types"):
        dt_list = value.data_types
        if dt_list and _is_pybind11_enum(dt_list[0]):
            items = [_encode_enum(d) for d in dt_list]
            lines.append(f"{var}.data_types = [{', '.join(items)}]")
        else:
            lines.append(f"{var}.data_types = {repr(dt_list)}")
    if hasattr(value, "skipped_array_mask"):
        lines.append(f"{var}.skipped_array_mask = {repr(value.skipped_array_mask)}")
    if hasattr(value, "tuple_dims"):
        lines.append(f"{var}.tuple_dims = {repr(value.tuple_dims)}")
    if hasattr(value, "headers_line"):
        lines.append(f"{var}.headers_line = {repr(value.headers_line)}")
    if hasattr(value, "header_mode") and _is_pybind11_enum(value.header_mode):
        lines.append(f"{var}.header_mode = {_encode_enum(value.header_mode)}")
    lines.append(var)
    return lines


def _encode_oem_ebsd_scan(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode OEMEbsdScanSelectionParameter.ValueType construction."""
    var = context.unique_name("oem_param")
    lines = [f"{var} = nxor.OEMEbsdScanSelectionParameter.ValueType()"]
    lines.append(f"{var}.input_file_path = {repr(str(value.input_file_path))}")
    lines.append(f"{var}.stacking_order = {repr(value.stacking_order)}")
    lines.append(f"{var}.scan_names = {repr(list(value.scan_names))}")
    lines.append(var)
    return lines


def _encode_crop_geometry(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode CropGeometryParameter.CropValues construction."""
    var = context.unique_name("crop_values")
    lines = [f"{var} = nx.CropGeometryParameter.CropValues()"]
    if hasattr(value, "type") and _is_pybind11_enum(value.type):
        lines.append(f"{var}.type = {_encode_enum(value.type)}")
    for attr in ("is_2d", "crop_x", "crop_y", "crop_z"):
        if hasattr(value, attr):
            lines.append(f"{var}.{attr} = {repr(getattr(value, attr))}")
    for attr in ("x_bound_voxels", "y_bound_voxels", "z_bound_voxels",
                  "x_bound_physical", "y_bound_physical", "z_bound_physical"):
        if hasattr(value, attr):
            lines.append(f"{var}.{attr} = {repr(list(getattr(value, attr)))}")
    lines.append(var)
    return lines


def _encode_dynamic_table(name: str, value: Any, context: CodeGenContext) -> list[str]:
    """Encode DynamicTableParameter values (2D list of floats)."""
    if len(value) == 1 and len(value[0]) <= 5:
        return [repr(value)]
    lines_inner = ",\n    ".join(repr(row) for row in value)
    return [f"[\n    {lines_inner}\n]"]


# ---------------------------------------------------------------------------
# Parameter UUID -> codec mapping
# ---------------------------------------------------------------------------

# Maps parameter type UUID -> encoder function.
# Parameters not in this map use _encode_default.
_PARAMETER_CODECS: dict[nx.Uuid, Any] = {
    nx.Uuid("e93251bc-cdad-44c2-9332-58fe26aedfbe") : _encode_array_threshold_set,     # ArrayThresholdsParameter
    nx.Uuid("aac15aa6-b367-508e-bf73-94ab6be0058b") : _encode_generated_file_list,     # GeneratedFileListParameter
    nx.Uuid("fac15aa6-b367-508e-bf73-94ab6be0058b") : _encode_read_h5ebsd,             # ReadH5EbsdFileParameter
    nx.Uuid("4f6d6a33-48da-427a-8b17-61e07d1d5b45") : _encode_read_csv_data,           # ReadCSVFileParameter
    nx.Uuid("ba2d4937-dbec-5536-8c5c-c0a406e80f77") : _encode_calculator,              # CalculatorParameter
    nx.Uuid("170a257d-5952-4854-9a91-4281cd06f4f5") : _encode_dream3d_import,          # Dream3dImportParameter
    nx.Uuid("32e83e13-ee4c-494e-8bab-4e699df74a5a") : _encode_read_hdf5_dataset,       # ReadHDF5DatasetParameter
    nx.Uuid("3935c833-aa51-4a58-81e9-3a51972c05ea") : _encode_oem_ebsd_scan,           # OEMEbsdScanSelectionParameter
    nx.Uuid("32b03ebf-02a5-40c7-a41c-2380722caeb7") : _encode_crop_geometry,           # CropGeometryParameter
    nx.Uuid("eea76f1a-fab9-4704-8da5-4c21057cf44e") : _encode_dynamic_table,           # DynamicTableParameter
}


def _find_codec(param_uuid: nx.Uuid) -> Any:
    """Look up encoder function by parameter UUID, falling back to default."""
    return _PARAMETER_CODECS.get(param_uuid, _encode_default)


def _resolve_filter_module(filter_obj: Any) -> tuple[str, str, str]:
    """Return (module_name, module_alias, class_name) for a filter object."""
    module_name = type(filter_obj).__module__
    class_name = type(filter_obj).__name__
    alias = MODULE_ALIASES.get(module_name, module_name)
    return module_name, alias, class_name

def _get_needed_modules(filters: list) -> set[str]:
    """Import all plugin modules so their parameter types are registered.

    Returns the set of needed module names.
    """
    needed_modules: set[str] = {"simplnx"}
    for pipeline_filter in filters:
        f = pipeline_filter.get_filter()
        mod_name, _, _ = _resolve_filter_module(f)
        needed_modules.add(mod_name)

    return needed_modules

 
def _build_import_lines(needed_modules: set[str]) -> list[str]:
    """Build import statements for the generated script."""
    import_lines = []
    for mod in _MODULE_ORDER:
        if mod in needed_modules:
            alias = MODULE_ALIASES.get(mod, mod)
            import_lines.append(f"import {mod} as {alias}")
    # Any unknown modules not in the canonical order
    for mod in sorted(needed_modules):
        if mod not in MODULE_ALIASES:
            import_lines.append(f"import {mod}")
    return import_lines


def _generate_filter_block(pipeline_filter: nx.PipelineFilter, context: CodeGenContext) -> list[str]:
    """Generate code lines for a single PipelineFilter."""
    index = context.filter_index
    f = pipeline_filter.get_filter()
    _, alias, class_name = _resolve_filter_module(f)
    human_name = pipeline_filter.human_name()
    args = pipeline_filter.get_args()
    params = f.parameters()

    lines: list[str] = []
    lines.append(f"# Filter {index}: {human_name}")

    setup_lines: list[str] = []
    kwargs: list[tuple[str, str]] = []

    for arg_name in sorted(args.keys()):
        value = args[arg_name]
        param_uuid = params[arg_name].uuid
        codec = _find_codec(param_uuid)
        encoded = codec(arg_name, value, context)

        if len(encoded) == 1:
            kwargs.append((arg_name, encoded[0]))
        else:
            setup_lines.extend(encoded[:-1])
            kwargs.append((arg_name, encoded[-1]))

    lines.extend(setup_lines)

    lines.append(f"result = {alias}.{class_name}.execute(")
    lines.append("    data_structure=data_structure,")
    for arg_name, expr in kwargs:
        lines.append(f"    {arg_name}={expr},")
    lines.append(")")
    lines.append(f"simplnx_utilities.check_filter_result({alias}.{class_name}, result)")

    return lines

def _generate_full(filters: list[nx.PipelineFilter]) -> str:
    """Assemble imports + boilerplate + filter blocks + footer."""
    context = CodeGenContext(filter_index=1)
    sections: list[str] = []

    needed_modules = _get_needed_modules(filters)
    needed_modules.add("simplnx_utilities")

    # Imports
    sections.append("\n".join(_build_import_lines(needed_modules)))

    # DataStructure creation
    sections.append("\n\ndata_structure = nx.DataStructure()")

    # Filter blocks
    for pipeline_filter in filters:
        block = _generate_filter_block(pipeline_filter, context)
        sections.append("\n".join(block))
        context.filter_index += 1

    # Footer
    sections.append('print("===> Pipeline Complete")')

    return "\n\n".join(sections) + "\n"


# ---------------------------------------------------------------------------
# Public API
# ---------------------------------------------------------------------------

def check_filter_result(filter: nx.IFilter, result: nx.IFilter.ExecuteResult) -> None:
  if len(result.warnings) != 0:
    print(f'{filter.name()} ::  Warnings: {result.warnings}')
  if len(result.errors) != 0:
    print(f'{filter.name()} :: Errors: {result.errors}')
    raise RuntimeError(result)
  print(f"{filter.name()} :: No errors running the filter")

def generate_python_pipeline(pipeline: nx.Pipeline) -> str:
    """Generate a full runnable Python script from a Pipeline object.

    Returns a string containing the complete script with imports,
    DataStructure creation, filter execution blocks, and footer.
    """
    filters = [pipeline[i] for i in range(len(pipeline))]
    return _generate_full(filters)


def generate_python_filters(filters: list[nx.PipelineFilter]) -> str:
    """Generate just the filter execution blocks for clipboard/UI use.

    No imports, DataStructure creation, or footer — just the filter blocks.
    """
    context = CodeGenContext(filter_index=1)
    blocks: list[str] = []
    for pipeline_filter in filters:
        block = _generate_filter_block(pipeline_filter, context)
        blocks.append("\n".join(block))
        context.filter_index += 1
    return "\n\n".join(blocks) + "\n"
