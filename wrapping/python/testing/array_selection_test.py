import importlib
from pathlib import Path
import sys
import tempfile
from types import ModuleType
import unittest
from unittest.mock import patch
import warnings

import simplnx as nx

from ExamplePlugin.InitializeData import InitializeDataPythonFilter


class SelectionFilter:
    def __init__(self, parameter):
        self.parameter = parameter

    def uuid(self):
        return nx.Uuid("e3dcefbc-6bd9-46a5-9aa3-dd22c21b046b")

    def name(self):
        return "SelectionFilter"

    class_name = human_name = name

    def default_tags(self):
        return []

    def parameters(self):
        parameters = nx.Parameters()
        parameters.insert(self.parameter)
        return parameters

    def parameters_version(self):
        return 1

    def preflight_impl(self, data_structure, args, message_handler, should_cancel):
        return nx.IFilter.PreflightResult(nx.OutputActions())


class ArraySelectionTest(unittest.TestCase):
    def setUp(self):
        self.data_structure = nx.DataStructure()

    def apply_action(self, action):
        result = action.apply(self.data_structure, nx.IDataAction.Mode.Execute)
        self.assertTrue(result.valid(), result.errors)

    def create_array(self, data_type, tuple_shape, *names):
        path = nx.DataPath(list(names))
        self.apply_action(nx.CreateArrayAction(data_type, tuple_shape, [1], path, ""))
        return path

    def assert_preflight_errors(self, filter_instance, expected_codes, **args):
        result = nx.PyFilter(filter_instance).preflight2(self.data_structure, **args)
        errors = result.get_result()
        self.assertEqual([error.code for error in errors], expected_codes, errors)

    def test_initialize_data_rejects_boolean_and_accepts_integer_and_float(self):
        image_path = nx.DataPath(["Image"])
        self.apply_action(nx.CreateImageGeometryAction(image_path, [2, 2, 2], [0, 0, 0], [1, 1, 1], "Cells"))
        integer = self.create_array(nx.DataType.int32, [2, 2, 2], "Image", "Cells", "Integer")
        floating = self.create_array(nx.DataType.float32, [2, 2, 2], "Image", "Cells", "Float")
        boolean = self.create_array(nx.DataType.boolean, [2, 2, 2], "Image", "Cells", "Boolean")
        args = {InitializeDataPythonFilter.IMAGE_GEOMETRY_PATH_KEY: image_path}
        self.assert_preflight_errors(InitializeDataPythonFilter(), [], cell_arrays=[integer, floating], **args)
        self.assert_preflight_errors(InitializeDataPythonFilter(), [-207], cell_arrays=[boolean], **args)

    def test_single_selection_legacy_overload(self):
        integer = self.create_array(nx.DataType.int32, [1], "Integer")
        string = nx.DataPath(["String"])
        self.apply_action(nx.CreateStringArrayAction([1], string))
        common = ("selected", "Selected Array", "", nx.DataPath())
        data_types = {nx.DataType.int32}
        location = nx.ArraySelectionParameter.DataLocation.InMemory
        for args, kwargs in (
            ((*common, data_types, [[1]], location), {}),
            ((), dict(name=common[0], human_name=common[1], help_text=common[2], default_value=common[3],
                      allowed_types=data_types, required_comps=[[1]], location=location)),
        ):
            with self.subTest(args=args, kwargs=kwargs):
                with self.assertWarnsRegex(DeprecationWarning, "allowed_array_types"):
                    parameter = nx.ArraySelectionParameter(*args, **kwargs)
                self.assert_preflight_errors(SelectionFilter(parameter), [], selected=integer)
                self.assert_preflight_errors(SelectionFilter(parameter), [-211], selected=string)

    def test_single_selection_explicit_kind_overload(self):
        string = nx.DataPath(["String"])
        self.apply_action(nx.CreateStringArrayAction([1], string))
        common = ("selected", "Selected Array", "", nx.DataPath())
        string_only = {nx.IArray.ArrayType.StringArray}
        for args, kwargs in (
            ((*common, string_only, set(), [[1]]), {}),
            (common, dict(allowed_array_types=string_only, allowed_data_types=set())),
        ):
            with self.subTest(args=args, kwargs=kwargs), warnings.catch_warnings():
                warnings.simplefilter("error", DeprecationWarning)
                parameter = nx.ArraySelectionParameter(*args, **kwargs)
                self.assert_preflight_errors(SelectionFilter(parameter), [], selected=string)

    def test_single_selection_legacy_empty_element_types_raise(self):
        with self.assertWarnsRegex(DeprecationWarning, "allowed_array_types"):
            with self.assertRaisesRegex(RuntimeError, "nonempty AllowedDataTypes"):
                nx.ArraySelectionParameter("selected", "Selected Array", "", nx.DataPath(), set())

    def test_single_selection_explicit_empty_element_types_raise(self):
        kind = nx.IArray.ArrayType
        for kinds in ({kind.DataArray}, {kind.NeighborListArray}, {kind.StringArray, kind.DataArray}):
            with self.subTest(kinds=kinds):
                with self.assertRaisesRegex(ValueError, "allowed_data_types.+element types"):
                    nx.ArraySelectionParameter("selected", "Selected Array", "", nx.DataPath(), kinds, set())

    def test_array_kind_helpers(self):
        kind = nx.IArray.ArrayType
        self.assertEqual(nx.get_data_array_only_types(), {kind.DataArray})
        self.assertEqual(nx.get_numeric_array_types(), {kind.DataArray, kind.NeighborListArray})
        self.assertEqual(nx.get_all_array_types(), {kind.DataArray, kind.NeighborListArray, kind.StringArray})

    def test_multi_selection_accepts_both_keywords_without_warning(self):
        integer = self.create_array(nx.DataType.int32, [1], "Integer")
        string = nx.DataPath(["String"])
        self.apply_action(nx.CreateStringArrayAction([1], string))
        for keyword in ("allowed_types", "allowed_array_types"):
            with self.subTest(keyword=keyword), warnings.catch_warnings():
                warnings.simplefilter("error", DeprecationWarning)
                parameter = nx.MultiArraySelectionParameter(
                    "selected", "Selected Arrays", "", [], allowed_data_types={nx.DataType.int32},
                    **{keyword: {nx.IArray.ArrayType.Any}})
                self.assert_preflight_errors(SelectionFilter(parameter), [], selected=[integer, string])

    def test_multi_selection_empty_element_types_allow_all_with_warning(self):
        kind = nx.IArray.ArrayType
        integer = self.create_array(nx.DataType.int32, [1], "Integer")
        boolean = self.create_array(nx.DataType.boolean, [1], "Boolean")
        neighbors = nx.DataPath(["Neighbors"])
        self.apply_action(nx.CreateNeighborListAction(nx.DataType.float32, [1], neighbors))
        for keyword in ("allowed_types", "allowed_array_types"):
            for array_kind, paths in (
                (kind.DataArray, [integer, boolean]),
                (kind.NeighborListArray, [neighbors]),
                (kind.Any, [integer, boolean, neighbors]),
            ):
                with self.subTest(keyword=keyword, array_kind=array_kind):
                    with self.assertWarnsRegex(DeprecationWarning, "allowed_data_types"):
                        parameter = nx.MultiArraySelectionParameter(
                            "selected", "Selected Arrays", "", [], allowed_data_types=set(),
                            **{keyword: {array_kind}})
                    self.assert_preflight_errors(SelectionFilter(parameter), [], selected=paths)

    def test_multi_selection_string_only_empty_element_types_without_warning(self):
        string = nx.DataPath(["String"])
        self.apply_action(nx.CreateStringArrayAction([1], string))
        integer = self.create_array(nx.DataType.int32, [1], "Integer")
        with warnings.catch_warnings():
            warnings.simplefilter("error", DeprecationWarning)
            parameter = nx.MultiArraySelectionParameter(
                "selected", "Selected Arrays", "", [], {nx.IArray.ArrayType.StringArray}, set(), [[1]])
        self.assert_preflight_errors(SelectionFilter(parameter), [], selected=[string])
        self.assert_preflight_errors(SelectionFilter(parameter), [-211], selected=[integer])

    def test_mesh_writers_reject_unsupported_cell_element_types(self):
        # Exercise the real declarations without importing unrelated toolkit filters or writing mesh files.
        # A minimal meshio module also lets the utility's Mesh annotation load in core-only test environments.
        toolkit = ModuleType("_test_mesh_writers")
        toolkit.__path__ = [str(Path(__file__).resolve().parents[1] / "plugins" / "NXDataAnalysisToolkit"
                                / "src" / "NXDataAnalysisToolkit")]
        meshio = ModuleType("meshio")
        meshio.Mesh = object
        with patch.dict(sys.modules, {toolkit.__name__: toolkit, "meshio": meshio}):
            tetgen = importlib.import_module(f"{toolkit.__name__}.WriteTetGenFile").WriteTetGenFile
            vtu = importlib.import_module(f"{toolkit.__name__}.WriteVtuFile").WriteVtuFile

        mesh_path = nx.DataPath(["Mesh"])
        self.apply_action(nx.CreateTetrahedralGeometryAction(mesh_path, 1, 4, "Vertices", "Cells", "Coords", "Tetrahedra"))
        integer = self.create_array(nx.DataType.int32, [1], "Mesh", "Cells", "Integer")
        floating = self.create_array(nx.DataType.float32, [1], "Mesh", "Cells", "Float")
        boolean = self.create_array(nx.DataType.boolean, [1], "Mesh", "Cells", "Boolean")
        with tempfile.TemporaryDirectory() as output_dir:
            for writer, suffix, rejected in ((tetgen, ".ele", floating), (vtu, ".vtu", boolean)):
                with self.subTest(writer=writer.__name__):
                    args = {writer.INPUT_GEOMETRY_KEY: mesh_path,
                            writer.OUTPUT_FILE_PATH_KEY: Path(output_dir) / f"selected{suffix}"}
                    self.assert_preflight_errors(writer(), [], cell_data_array_paths=[integer], **args)
                    self.assert_preflight_errors(writer(), [-207], cell_data_array_paths=[rejected], **args)


if __name__ == "__main__":
    unittest.main()
