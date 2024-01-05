import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = nx.ReadStlFileFilter()
# Execute Filter with Parameters
result = nx_filter.execute(    
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_factor=1.0,
    scale_output=False,
    stl_file_path=nxtest.GetDataDirectory() + "/STL_Models/ASTMD638_specimen.stl",
    triangle_geometry_name=nx.DataPath("[Triangle Geometry]"),
    vertex_attribute_matrix="Vertex Data"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.CalculateTriangleAreasFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    triangle_areas_array_path="Face Areas",
    triangle_geometry_data_path=nx.DataPath("[Triangle Geometry]")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.TriangleNormalFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_normals_array_path="Face Normals (Calculated)",
    tri_geometry_data_path=nx.DataPath("[Triangle Geometry]")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.TriangleDihedralAngleFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_dihedral_angles_array_name="Dihedral Angles",
    tri_geometry_data_path=nx.DataPath("[Triangle Geometry]")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.TriangleCentroidFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    centroids_array_name="Centroids",
    triangle_geometry_path=nx.DataPath("[Triangle Geometry]")
)
nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")