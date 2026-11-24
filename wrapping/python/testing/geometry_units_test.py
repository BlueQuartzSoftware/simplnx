import unittest

import simplnx as nx


class ImageGeometryUnitsTest(unittest.TestCase):
    def test_image_geometry_action_requires_units(self):
        path = nx.DataPath(["Image"])
        with self.assertRaises(TypeError):
            nx.CreateImageGeometryAction(path, [2, 3, 4], [0.0] * 3, [1.0] * 3, "Cell Data")

    def test_image_geometry_action_applies_units(self):
        path = nx.DataPath(["Image"])
        for unit in (nx.IGeometry.LengthUnit.Millimeter, nx.IGeometry.LengthUnit.Unspecified):
            with self.subTest(unit=unit):
                data_structure = nx.DataStructure()
                action = nx.CreateImageGeometryAction(path, [2, 3, 4], [0.0] * 3, [1.0] * 3, "Cell Data", units=unit)
                result = action.apply(data_structure, nx.IDataAction.Mode.Execute)
                self.assertTrue(result.valid(), [str(error) for error in result.errors])
                self.assertEqual(data_structure[path].units, unit)


if __name__ == "__main__":
    unittest.main()
