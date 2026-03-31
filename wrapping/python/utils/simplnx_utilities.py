"""Simplnx utilities for converting live Pipeline objects to runnable Python scripts.

Usage:
    import simplnx as nx
    from simplnx_utilities import create_default_generator

    pipeline = nx.Pipeline.from_file("path/to/pipeline.d3dpipeline")
    generator = create_default_generator()
    print(generator.generate(pipeline))
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Protocol

# ---------------------------------------------------------------------------
# Module alias map (no legacy nx/simplnx terminology)
# ---------------------------------------------------------------------------

MODULE_ALIASES: dict[str, str] = {
    "simplnx": "nx",
    "orientationanalysis": "nxor",
    "itkimageprocessing": "nxitk",
}

MODULE_IMPORTS: dict[str, str] = {
    "simplnx": "import simplnx as nx",
    "orientationanalysis": "import orientationanalysis as nxor",
    "itkimageprocessing": "import itkimageprocessing as nxitk",
}


# ---------------------------------------------------------------------------
# CodeGenContext — lightweight state passed to codecs
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
# ValueCodec protocol and registry
# ---------------------------------------------------------------------------

class ValueCodec(Protocol):
    """Interface for converting a Python value into lines of code."""

    def can_encode(self, value: Any) -> bool: ...

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        """Return lines of Python source.

        Convention:
        - All lines except the last are setup statements.
        - The last line is the value expression to use as the kwarg.
        """
        ...


class CodecRegistry:
    """Ordered collection of codecs — first match wins."""

    def __init__(self) -> None:
        self._codecs: list[ValueCodec] = []
        self._default = DefaultCodec()

    def register(self, codec: ValueCodec) -> None:
        self._codecs.append(codec)

    def find(self, value: Any) -> ValueCodec:
        for codec in self._codecs:
            if codec.can_encode(value):
                return codec
        return self._default


# ---------------------------------------------------------------------------
# FilterModuleResolver
# ---------------------------------------------------------------------------

class FilterModuleResolver:
    """Maps a filter object to its Python module, alias, and class name."""

    def resolve(self, filter_obj: Any) -> tuple[str, str, str]:
        """Return (module_name, module_alias, class_name).

        Example: ('orientationanalysis', 'nxor', 'ReadH5EbsdFilter')
        """
        module_name = type(filter_obj).__module__
        class_name = type(filter_obj).__name__
        alias = MODULE_ALIASES.get(module_name, module_name)
        return module_name, alias, class_name


# ---------------------------------------------------------------------------
# Enum and DataPath helpers
# ---------------------------------------------------------------------------

def _is_pybind11_enum(value: Any) -> bool:
    """Check if a value is a pybind11-bound enum."""
    t = type(value)
    return hasattr(t, "__members__") and hasattr(value, "name") and hasattr(value, "value")


def _encode_enum(value: Any) -> str:
    """Produce e.g. 'nx.DataType.boolean' or 'nx.ArrayThreshold.ComparisonType.GreaterThan'."""
    t = type(value)
    module_name = t.__module__
    alias = MODULE_ALIASES.get(module_name, module_name)
    qualname = t.__qualname__
    return f"{alias}.{qualname}.{value.name}"


def _is_datapath(value: Any) -> bool:
    """Check if a value is a simplnx DataPath."""
    return type(value).__name__ == "DataPath" and hasattr(value, "to_string")


def _encode_datapath(value: Any) -> str:
    """Produce e.g. 'nx.DataPath("DataContainer/Cell Data")'."""
    path_str = value.to_string("/")
    # Use repr() for the string to safely handle embedded quotes
    return f"nx.DataPath({repr(path_str)})"


def _encode_simple_value(value: Any) -> str:
    """Encode a single simple value to a Python expression string."""
    import pathlib
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
    # If the value round-trips cleanly through 6 significant digits,
    # use that (handles float32 → float64 artifacts like 0.20000000298023224 → 0.2)
    for digits in (6, 8, 10):
        rounded = round(value, digits)
        if abs(rounded - value) < abs(value) * 1e-7 or abs(rounded - value) < 1e-12:
            # Use the shortest clean representation
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
# DefaultCodec — handles all simple/primitive types via repr
# ---------------------------------------------------------------------------

class DefaultCodec:
    """Fallback codec using repr() with special handling for DataPath and enums."""

    def can_encode(self, value: Any) -> bool:
        return True

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        return [_encode_simple_value(value)]


# ---------------------------------------------------------------------------
# ArrayThresholdSetCodec
# ---------------------------------------------------------------------------

class ArrayThresholdSetCodec:
    """Generates ArrayThreshold construction + ArrayThresholdSet assembly."""

    def can_encode(self, value: Any) -> bool:
        return type(value).__name__ == "ArrayThresholdSet"

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        lines: list[str] = []
        threshold_vars: list[str] = []

        for threshold in value.thresholds:
            var = ctx.unique_name("threshold")
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

        set_var = ctx.unique_name("threshold_set")
        lines.append(f"{set_var} = nx.ArrayThresholdSet()")
        lines.append(f"{set_var}.thresholds = [{', '.join(threshold_vars)}]")
        if value.inverted:
            lines.append(f"{set_var}.inverted = True")
        if value.union_op.name != "And":
            lines.append(f"{set_var}.union_op = {_encode_enum(value.union_op)}")

        lines.append(set_var)  # expression line
        return lines


# ---------------------------------------------------------------------------
# GeneratedFileListCodec
# ---------------------------------------------------------------------------

class GeneratedFileListCodec:
    """Generates GeneratedFileListParameter.ValueType construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__qualname__ == "GeneratedFileListParameter.ValueType"
                or (type(value).__name__ == "ValueType"
                    and hasattr(value, "input_path")
                    and hasattr(value, "file_prefix")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("file_list")
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


# ---------------------------------------------------------------------------
# ReadH5EbsdValueCodec
# ---------------------------------------------------------------------------

class ReadH5EbsdValueCodec:
    """Generates ReadH5EbsdFileParameter.ValueType construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__qualname__ == "ReadH5EbsdFileParameter.ValueType"
                or (type(value).__name__ == "ValueType"
                    and hasattr(value, "euler_representation")
                    and hasattr(value, "start_slice")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("h5ebsd_param")
        lines = [f"{var} = nxor.ReadH5EbsdFileParameter.ValueType()"]
        lines.append(f"{var}.input_file_path = {repr(value.input_file_path)}")
        lines.append(f"{var}.start_slice = {repr(value.start_slice)}")
        lines.append(f"{var}.end_slice = {repr(value.end_slice)}")
        lines.append(f"{var}.euler_representation = {repr(value.euler_representation)}")
        lines.append(f"{var}.selected_array_names = {repr(value.selected_array_names)}")
        lines.append(f"{var}.use_recommended_transform = {repr(value.use_recommended_transform)}")
        lines.append(var)
        return lines


# ---------------------------------------------------------------------------
# CalculatorValueCodec
# ---------------------------------------------------------------------------

class CalculatorValueCodec:
    """Generates CalculatorParameter.ValueType construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__qualname__ == "CalculatorParameter.ValueType"
                or (type(value).__name__ == "ValueType"
                    and hasattr(value, "equation")
                    and hasattr(value, "units")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("calc_param")
        selected_group = _encode_datapath(value.selected_group) if _is_datapath(value.selected_group) else repr(value.selected_group)
        units = _encode_enum(value.units) if _is_pybind11_enum(value.units) else repr(value.units)
        lines = [f"{var} = nx.CalculatorParameter.ValueType({selected_group}, {repr(value.equation)}, {units})"]
        lines.append(var)
        return lines


# ---------------------------------------------------------------------------
# Dream3dImportCodec
# ---------------------------------------------------------------------------

class Dream3dImportCodec:
    """Generates Dream3dImportParameter.ImportData construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__qualname__ == "Dream3dImportParameter.ImportData"
                or type(value).__name__ == "ImportData")

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("import_data")
        lines = [f"{var} = nx.Dream3dImportParameter.ImportData()"]
        lines.append(f"{var}.file_path = {repr(str(value.file_path))}")
        if hasattr(value, "data_paths") and value.data_paths:
            paths_str = _encode_list([dp for dp in value.data_paths])
            lines.append(f"{var}.data_paths = {paths_str}")
        if hasattr(value, "import_policy") and _is_pybind11_enum(value.import_policy):
            lines.append(f"{var}.import_policy = {_encode_enum(value.import_policy)}")
        lines.append(var)
        return lines


# ---------------------------------------------------------------------------
# ReadHDF5DatasetCodec
# ---------------------------------------------------------------------------

class ReadHDF5DatasetCodec:
    """Generates ReadHDF5DatasetParameter.ValueType construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__qualname__ == "ReadHDF5DatasetParameter.ValueType"
                or (type(value).__name__ == "ValueType"
                    and hasattr(value, "input_file")
                    and hasattr(value, "datasets")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("hdf5_param")
        lines = [f"{var} = nx.ReadHDF5DatasetParameter.ValueType()"]
        lines.append(f"{var}.input_file = {repr(value.input_file)}")
        if hasattr(value, "parent") and value.parent is not None and _is_datapath(value.parent):
            lines.append(f"{var}.parent = {_encode_datapath(value.parent)}")
        if hasattr(value, "datasets"):
            dataset_lines = []
            for ds in value.datasets:
                ds_var = ctx.unique_name("dataset_info")
                lines.append(f"{ds_var} = nx.DatasetImportInfo()")
                lines.append(f"{ds_var}.data_set_path = {repr(ds.data_set_path)}")
                lines.append(f"{ds_var}.component_dimensions = {repr(ds.component_dimensions)}")
                lines.append(f"{ds_var}.tuple_dimensions = {repr(ds.tuple_dimensions)}")
                dataset_lines.append(ds_var)
            lines.append(f"{var}.datasets = [{', '.join(dataset_lines)}]")
        lines.append(var)
        return lines


# ---------------------------------------------------------------------------
# ReadCSVDataCodec
# ---------------------------------------------------------------------------

class ReadCSVDataCodec:
    """Generates ReadCSVData construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__name__ == "ReadCSVData"
                or (type(value).__name__ == "ValueType"
                    and hasattr(value, "input_file_path")
                    and hasattr(value, "delimiters")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("csv_data")
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


# ---------------------------------------------------------------------------
# OEMEbsdScanCodec
# ---------------------------------------------------------------------------

class OEMEbsdScanCodec:
    """Generates OEMEbsdScanSelectionParameter.ValueType construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__qualname__ == "OEMEbsdScanSelectionParameter.ValueType"
                or (type(value).__name__ == "ValueType"
                    and hasattr(value, "scan_names")
                    and hasattr(value, "stacking_order")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("oem_param")
        lines = [f"{var} = nxor.OEMEbsdScanSelectionParameter.ValueType()"]
        lines.append(f"{var}.input_file_path = {repr(str(value.input_file_path))}")
        lines.append(f"{var}.stacking_order = {repr(value.stacking_order)}")
        lines.append(f"{var}.scan_names = {repr(list(value.scan_names))}")
        lines.append(var)
        return lines


# ---------------------------------------------------------------------------
# CropGeometryCodec
# ---------------------------------------------------------------------------

class CropGeometryCodec:
    """Generates CropGeometryParameter.CropValues construction."""

    def can_encode(self, value: Any) -> bool:
        return (type(value).__name__ == "CropValues"
                or (hasattr(value, "crop_x") and hasattr(value, "x_bound_voxels")))

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        var = ctx.unique_name("crop_values")
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


# ---------------------------------------------------------------------------
# DynamicTableCodec
# ---------------------------------------------------------------------------

class DynamicTableCodec:
    """Handles DynamicTableParameter values (2D list of floats)."""

    def can_encode(self, value: Any) -> bool:
        if not isinstance(value, list) or not value:
            return False
        return isinstance(value[0], list) and all(
            isinstance(row, list) and all(isinstance(x, (int, float)) for x in row)
            for row in value
        )

    def encode(self, name: str, value: Any, ctx: CodeGenContext) -> list[str]:
        if len(value) == 1 and len(value[0]) <= 5:
            return [repr(value)]
        lines_inner = ",\n    ".join(repr(row) for row in value)
        return [f"[\n    {lines_inner}\n]"]


# ---------------------------------------------------------------------------
# PipelineCodeGenerator — the orchestrator
# ---------------------------------------------------------------------------

class PipelineCodeGenerator:
    """Converts Pipeline/PipelineFilter objects to Python source code."""

    def __init__(self, registry: CodecRegistry | None = None) -> None:
        self._registry = registry or _create_default_registry()
        self._resolver = FilterModuleResolver()

    # -- public API --

    def generate(self, pipeline: Any) -> str:
        """Full runnable script from a Pipeline object."""
        filters = [pipeline[i] for i in range(len(pipeline))]
        return self._generate_full(filters)

    def generate_filters(self, filters: list[Any]) -> str:
        """Just the filter execution blocks (for clipboard/UI use)."""
        ctx = CodeGenContext()
        blocks: list[str] = []
        for i, pf in enumerate(filters):
            ctx.filter_index = i + 1
            block = self._generate_filter(pf, i + 1, ctx)
            blocks.append("\n".join(block))
        return "\n\n".join(blocks) + "\n"

    # -- internal --

    def _generate_full(self, filters: list[Any]) -> str:
        """Assemble imports + boilerplate + filter blocks + footer."""
        ctx = CodeGenContext()
        sections: list[str] = []

        # Collect needed modules
        needed_modules: set[str] = {"simplnx"}
        for pf in filters:
            f = pf.get_filter()
            mod_name, _, _ = self._resolver.resolve(f)
            needed_modules.add(mod_name)

        # Imports
        import_lines = []
        for mod in ("simplnx", "orientationanalysis", "itkimageprocessing"):
            if mod in needed_modules and mod in MODULE_IMPORTS:
                import_lines.append(MODULE_IMPORTS[mod])
        # Add any unknown modules
        for mod in sorted(needed_modules):
            if mod not in MODULE_IMPORTS:
                import_lines.append(f"import {mod}")
        sections.append("\n".join(import_lines))
        
        # Append the check_filter_result() function
        sections.append("\n".join([
            "import os",
            "import shutil",
            "from pathlib import Path",
            "",
            "def check_filter_result(filter: nx.IFilter, result: nx.IFilter.ExecuteResult) -> None:",
            "  if len(result.warnings) != 0:",
            "    print(f'{filter.name()} ::  Warnings: {result.warnings}')",
            "  has_errors = len(result.errors) != 0",
            "  if has_errors:",
            "    print(f'{filter.name()} :: Errors: {result.errors}')",
            "    raise RuntimeError(result)",
            "  print(f\"{filter.name()} :: No errors running the filter\")",
        ]))

        # DataStructure creation
        sections.append("\n\ndata_structure = nx.DataStructure()")

        # Filter blocks
        for i, pf in enumerate(filters):
            ctx.filter_index = i + 1
            block = self._generate_filter(pf, i + 1, ctx)
            sections.append("\n".join(block))

        # Footer
        sections.append('print("===> Pipeline Complete")')

        return "\n\n".join(sections) + "\n"

    def _generate_filter(self, pf: Any, index: int, ctx: CodeGenContext) -> list[str]:
        """Generate code lines for a single PipelineFilter."""
        f = pf.get_filter()
        _, alias, class_name = self._resolver.resolve(f)
        human_name = pf.human_name()
        args = pf.get_args()

        lines: list[str] = []
        lines.append(f"# Filter {index}: {human_name}")

        # Separate setup lines from inline kwargs
        setup_lines: list[str] = []
        kwargs: list[tuple[str, str]] = []

        for arg_name in sorted(args.keys()):
            value = args[arg_name]
            codec = self._registry.find(value)
            encoded = codec.encode(arg_name, value, ctx)

            if len(encoded) == 1:
                # Single expression — use as inline kwarg
                kwargs.append((arg_name, encoded[0]))
            else:
                # Multiple lines: setup statements + final expression
                setup_lines.extend(encoded[:-1])
                kwargs.append((arg_name, encoded[-1]))

        lines.extend(setup_lines)

        # Build the execute call
        lines.append(f"result = {alias}.{class_name}.execute(")
        lines.append(f"    data_structure=data_structure,")
        for arg_name, expr in kwargs:
            lines.append(f"    {arg_name}={expr},")
        lines.append(f")")
        lines.append(f"check_filter_result({alias}.{class_name}, result)")

        return lines


# ---------------------------------------------------------------------------
# Factory
# ---------------------------------------------------------------------------

def _create_default_registry() -> CodecRegistry:
    """Build a registry with all built-in codecs."""
    registry = CodecRegistry()
    registry.register(ArrayThresholdSetCodec())
    registry.register(GeneratedFileListCodec())
    registry.register(ReadH5EbsdValueCodec())
    registry.register(ReadCSVDataCodec())
    registry.register(CalculatorValueCodec())
    registry.register(Dream3dImportCodec())
    registry.register(ReadHDF5DatasetCodec())
    registry.register(OEMEbsdScanCodec())
    registry.register(CropGeometryCodec())
    registry.register(DynamicTableCodec())
    return registry


def create_default_generator() -> PipelineCodeGenerator:
    """Create a PipelineCodeGenerator with all built-in codecs registered."""
    return PipelineCodeGenerator(_create_default_registry())
