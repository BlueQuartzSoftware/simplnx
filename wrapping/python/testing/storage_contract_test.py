import unittest

import numpy as np

import simplnx as nx


MAX_TUPLE_COUNT = int(np.iinfo(np.intp).max)
RESIZE_ERROR_CODE = "-6035"


def data_path(*names):
    return nx.DataPath(list(names))


class StorageContractTest(unittest.TestCase):
    def apply_action(self, action, data_structure):
        result = action.apply(data_structure, nx.IDataAction.Mode.Execute)
        self.assertTrue(result.valid(), [str(error) for error in result.errors])

    def assert_resize_error(self, operation):
        with self.assertRaises(RuntimeError) as context:
            operation()
        message = str(context.exception)
        self.assertIn(RESIZE_ERROR_CODE, message)
        return message

    def test_data_store_resize_retains_values_and_reports_allocation_failure(self):
        store = nx.Int32DataStore([2], [1], 7)
        view = store.npview()
        view.reshape(-1)[:] = [11, 22]
        del view

        self.assertIsNone(store.resize_tuples([4]))
        self.assertEqual(list(store.tdims), [4])
        self.assertEqual([store[index] for index in range(len(store))], [11, 22, 7, 7])

        self.assert_resize_error(lambda: store.resize_tuples([MAX_TUPLE_COUNT]))
        self.assertEqual(list(store.tdims), [4])
        self.assertEqual([store[index] for index in range(len(store))], [11, 22, 7, 7])

    def test_numeric_array_resize_retains_values_and_reports_allocation_failure(self):
        data_structure = nx.DataStructure()
        array_path = data_path("NumericArray")
        self.apply_action(nx.CreateArrayAction(nx.DataType.int32, [2], [1], array_path, "", "5"), data_structure)
        array = data_structure[array_path]

        view = array.npview()
        view.reshape(-1)[:] = [31, 32]
        del view

        self.assertIsNone(array.resize_tuples([3]))
        self.assertEqual(list(array.tdims), [3])
        view = array.npview()
        self.assertEqual(view.reshape(-1).tolist(), [31, 32, 5])
        del view

        self.assert_resize_error(lambda: array.resize_tuples([MAX_TUPLE_COUNT]))
        self.assertEqual(list(array.tdims), [3])
        view = array.npview()
        self.assertEqual(view.reshape(-1).tolist(), [31, 32, 5])
        del view

    def test_string_array_resize_retains_values_and_reports_allocation_failure(self):
        data_structure = nx.DataStructure()
        array_path = data_path("Strings")
        self.apply_action(nx.CreateStringArrayAction([2], array_path, "initial"), data_structure)
        array = data_structure[array_path]
        array[0] = "first"
        array[1] = "second"

        self.assertIsNone(array.resize_tuples([3]))
        self.assertEqual(list(array.tdims), [3])
        self.assertEqual(array.values, ["first", "second", ""])

        self.assert_resize_error(lambda: array.resize_tuples([MAX_TUPLE_COUNT]))
        self.assertEqual(list(array.tdims), [3])
        self.assertEqual(array.values, ["first", "second", ""])

    def test_attribute_matrix_resize_propagates_numeric_child_failure(self):
        data_structure = nx.DataStructure()
        matrix_path = data_path("FeatureData")
        array_path = data_path("FeatureData", "Values")
        self.apply_action(nx.CreateAttributeMatrixAction(matrix_path, [2]), data_structure)
        self.apply_action(nx.CreateArrayAction(nx.DataType.int32, [2], [1], array_path, "", "9"), data_structure)

        matrix = data_structure[matrix_path]
        array = data_structure[array_path]
        view = array.npview()
        view[0] = 41
        del view

        self.assertIsNone(matrix.resize_tuples([3]))
        self.assertEqual(list(matrix.tuple_shape), [3])
        view = array.npview()
        self.assertEqual(view.reshape(-1).tolist(), [41, 9, 9])
        del view

        self.assert_resize_error(lambda: matrix.resize_tuples([MAX_TUPLE_COUNT]))
        self.assertEqual(list(array.tdims), [3])
        view = array.npview()
        self.assertEqual(view.reshape(-1).tolist(), [41, 9, 9])
        del view

    def test_geometry_role_resizes_update_shared_list_and_attribute_matrix(self):
        data_structure = nx.DataStructure()

        vertex_path = data_path("VertexGeometry")
        self.apply_action(nx.CreateVertexGeometryAction(vertex_path, 2, "VertexData", "Vertices"), data_structure)
        vertex_geometry = data_structure[vertex_path]
        self.assertIsNone(vertex_geometry.resize_vertices(5))
        self.assertEqual(list(vertex_geometry.vertices.tdims), [5])
        self.assertEqual(list(vertex_geometry.vertex_data.tuple_shape), [5])

        edge_path = data_path("EdgeGeometry")
        self.apply_action(nx.CreateEdgeGeometryAction(edge_path, 2, 3, "VertexData", "EdgeData", "Vertices", "Edges"), data_structure)
        edge_geometry = data_structure[edge_path]
        self.assertIsNone(edge_geometry.resize_edges(6))
        self.assertEqual(list(edge_geometry.edges.tdims), [6])
        self.assertEqual(list(edge_geometry.edge_data.tuple_shape), [6])

        triangle_path = data_path("TriangleGeometry")
        self.apply_action(nx.CreateTriangleGeometryAction(triangle_path, 2, 3, "VertexData", "FaceData", "Vertices", "Faces"), data_structure)
        triangle_geometry = data_structure[triangle_path]
        self.assertIsNone(triangle_geometry.resize_faces(7))
        self.assertEqual(list(triangle_geometry.faces.tdims), [7])
        self.assertEqual(list(triangle_geometry.face_data.tuple_shape), [7])

        hexahedral_path = data_path("HexahedralGeometry")
        self.apply_action(nx.CreateHexahedralGeometryAction(hexahedral_path, 2, 8, "VertexData", "PolyhedraData", "Vertices", "Polyhedra"), data_structure)
        hexahedral_geometry = data_structure[hexahedral_path]
        self.assertIsNone(hexahedral_geometry.resize_polyhedra(8))
        self.assertEqual(list(hexahedral_geometry.polyhedra.tdims), [8])
        self.assertEqual(list(hexahedral_geometry.polyhedra_data.tuple_shape), [8])

    def test_triangle_and_quad_resize_faces_updates_only_the_face_matrix(self):
        for geometry_type, name in ((nx.CreateTriangleGeometryAction, "TriangleGeometry"), (nx.CreateQuadGeometryAction, "QuadGeometry")):
            with self.subTest(geometry=name):
                data_structure = nx.DataStructure()
                geometry_path = data_path(name)
                edge_matrix_path = data_path(name, "SeparateEdgeData")
                self.apply_action(geometry_type(geometry_path, 2, 4, "VertexData", "FaceData", "Vertices", "Faces"), data_structure)
                self.apply_action(nx.CreateAttributeMatrixAction(edge_matrix_path, [2]), data_structure)
                geometry = data_structure[geometry_path]

                self.assertIsNone(geometry.resize_faces(5))
                self.assertEqual(list(geometry.faces.tdims), [5])
                self.assertEqual(list(geometry.face_data.tuple_shape), [5])
                self.assertEqual(list(data_structure[edge_matrix_path].tuple_shape), [2])

    def test_missing_geometry_matrices_raise_contextual_runtime_errors(self):
        scenarios = (
            (nx.CreateVertexGeometryAction, (2, "VertexData", "Vertices"), "VertexGeometry", "VertexData", "vertex", "resize_vertices"),
            (nx.CreateEdgeGeometryAction, (2, 3, "VertexData", "EdgeData", "Vertices", "Edges"), "EdgeGeometry", "EdgeData", "edge", "resize_edges"),
            (nx.CreateTriangleGeometryAction, (2, 4, "VertexData", "FaceData", "Vertices", "Faces"), "TriangleGeometry", "FaceData", "face", "resize_faces"),
            (nx.CreateHexahedralGeometryAction, (2, 8, "VertexData", "PolyhedraData", "Vertices", "Polyhedra"), "HexahedralGeometry", "PolyhedraData", "polyhedra", "resize_polyhedra"),
        )

        for action_type, arguments, geometry_name, matrix_name, role, resize_name in scenarios:
            with self.subTest(geometry=geometry_name, role=role):
                data_structure = nx.DataStructure()
                geometry_path = data_path(geometry_name)
                self.apply_action(action_type(geometry_path, *arguments), data_structure)
                geometry = data_structure[geometry_path]
                self.assertTrue(data_structure.remove(data_path(geometry_name, matrix_name)))

                with self.assertRaisesRegex(RuntimeError, rf"Geometry '{geometry_name}' has no {role} attribute matrix"):
                    getattr(geometry, resize_name)(5)

    def test_geometry_list_failure_preserves_its_attribute_matrix(self):
        data_structure = nx.DataStructure()
        geometry_path = data_path("VertexGeometry")
        self.apply_action(nx.CreateVertexGeometryAction(geometry_path, 2, "VertexData", "Vertices"), data_structure)
        geometry = data_structure[geometry_path]

        message = self.assert_resize_error(lambda: geometry.resize_vertices(MAX_TUPLE_COUNT))
        self.assertIn("DataStore resize to shape", message)
        self.assertEqual(list(geometry.vertex_data.tuple_shape), [2])

    def test_create_array_action_supports_legacy_and_explicit_fill_values(self):
        data_structure = nx.DataStructure()
        legacy_path = data_path("Legacy")
        zero_path = data_path("ZeroFill")
        filled_path = data_path("Filled")

        self.apply_action(nx.CreateArrayAction(nx.DataType.int32, [2], [2, 2], legacy_path), data_structure)
        self.apply_action(nx.CreateArrayAction(nx.DataType.int32, [2], [2, 2], zero_path, "", "0"), data_structure)
        self.apply_action(nx.CreateArrayAction(nx.DataType.int32, [2], [2, 2], filled_path, "", "17"), data_structure)

        self.assertEqual(list(data_structure[legacy_path].tdims), [2])
        self.assertEqual(list(data_structure[legacy_path].cdims), [2, 2])

        zero_view = data_structure[zero_path].npview()
        self.assertEqual(zero_view.shape, (2, 2, 2))
        self.assertEqual(zero_view.tolist(), [[[0, 0], [0, 0]], [[0, 0], [0, 0]]])
        del zero_view

        filled_view = data_structure[filled_path].npview()
        self.assertEqual(filled_view.shape, (2, 2, 2))
        self.assertEqual(filled_view.tolist(), [[[17, 17], [17, 17]], [[17, 17], [17, 17]]])
        del filled_view


if __name__ == "__main__":
    unittest.main()
