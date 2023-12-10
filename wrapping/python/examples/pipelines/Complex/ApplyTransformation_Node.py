import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.ReadStlFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cxtest.GetDataDirectory() + "Data/STL_Models/ASTMD638_specimen.stl",
    triangle_geometry_name=cx.DataPath("Blade"),
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
filter = cx.ReadStlFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    scale_output=False,
    stl_file_path=cxtest.GetDataDirectory() + "Data/STL_Models/ASTMD638_specimen.stl",
    triangle_geometry_name=cx.DataPath("Blade [Rotation]"),
    vertex_attribute_matrix="Vertex Data"
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
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #cell_attribute_matrix_path: DataPath = ...,
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=2,
    #manual_transformation_matrix: List[List[float]] = ...,
    rotation=[1.0, 0.0, 0.0, 180],
    #scale: List[float] = ...,
    selected_image_geometry=cx.DataPath("Blade [Rotation]"),
    transformation_type=3,
    translate_geometry_to_global_origin=False
    #translation: List[float] = ...
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
filter = cx.ReadStlFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cxtest.GetDataDirectory() + "Data/STL_Models/ASTMD638_specimen.stl",
    triangle_geometry_name=cx.DataPath("Blade [Translation]"),
    vertex_attribute_matrix="Vertex Data"
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
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    interpolation_type=2,
    selected_image_geometry=cx.DataPath("Blade [Translation]"),
    transformation_type=4,
    translate_geometry_to_global_origin=False,
    translation=[10.0, 10.0, 10.0]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cx.ReadStlFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cxtest.GetDataDirectory() + "Data/STL_Models/ASTMD638_specimen.stl",
    triangle_geometry_name=cx.DataPath("Blade [Scale]"),
    vertex_attribute_matrix="Vertex Data"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    interpolation_type=2,
    scale=[0.5, 0.5, 1.0],
    selected_image_geometry=cx.DataPath("Blade [Scale]"),
    transformation_type=5,
    translate_geometry_to_global_origin=False
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = cxtest.GetDataDirectory() + "Data/Output/ApplyTransformation_Node.dream3d"
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")