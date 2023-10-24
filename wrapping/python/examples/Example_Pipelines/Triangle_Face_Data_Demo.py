import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
result = cx.StlFileReaderFilter.execute(    
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_factor=1.0,
    scale_output=False,
    stl_file_path=(["Data/STL_Models/ASTMD638_specimen.stl"]),
    triangle_geometry_name=cx.DataPath("[Triange Geometry]"),
    vertex_attribute_matrix="Vertex Data",
)

#Filter 2
result = cx.CalculateTriangleAreasFilter.execute(
    data_structure=data_structure,
    triangle_areas_array_path="Face Areas",
    triangle_geometry_data_path=cx.DataPath("[Triange Geometry]"),
)
#Filter 3
result = cx.TriangleNormalFilter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_normals_array_path="Face Normals (Calculated)",
    tri_geometry_data_path=cx.DataPath("[Triange Geometry]")
)
#Filter 4
result = cx.TriangleDihedralAngleFilter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_dihedral_angles_array_name="Dihedral Angles",
    tri_geometry_data_path=cx.DataPath("[Triangle Geometry]")
)
#Filter 5
result = cx.TriangleCentroidFilter.execute(
    data_structure=data_structure,
    centroids_array_name="Centroids",
    triangle_geometry_path=cx.DataPath("[Triangle Geometry]")
)