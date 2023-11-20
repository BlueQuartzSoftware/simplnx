import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.ReadStlFileFilter()
# Execute Filter with Parameters
result = filter.execute(    
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_factor=1.0,
    scale_output=False,
    stl_file_path="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/STL_Models/ASTMD638_specimen.stl",
    triangle_geometry_name=cx.DataPath("[Triange Geometry]"),
    vertex_attribute_matrix="Vertex Data"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.CalculateTriangleAreasFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    triangle_areas_array_path="Face Areas",
    triangle_geometry_data_path=cx.DataPath("[Triange Geometry]")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.TriangleNormalFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_normals_array_path="Face Normals (Calculated)",
    tri_geometry_data_path=cx.DataPath("[Triange Geometry]")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.TriangleDihedralAngleFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_dihedral_angles_array_name="Dihedral Angles",
    tri_geometry_data_path=cx.DataPath("[Triangle Geometry]")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cx.TriangleCentroidFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_name="Centroids",
    triangle_geometry_path=cx.DataPath("[Triangle Geometry]")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")