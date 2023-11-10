import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.StlFileReaderFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
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
filter = cx.StlFileReaderFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
    triangle_geometry_name=cx.DataPath("Blade (Rotation)"),
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
    interpolation_type=0,
    rotation=[1.0, 0.0, 0.0, 180],
    selected_image_geometry=cx.DataPath("Blade Rotation"),
    transformation_type=3,
    translate_geometry_to_global_origin=False
    # translation: List[float] = ...  # Not used here
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
filter = cx.StlFileReaderFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
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
    interpolation_type=0,
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
filter = cx.StlFileReaderFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
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
    interpolation_type=0,
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
filter = cx.ExportDREAM3DFilter()
# Set Output File Path
output_file_path = "Data/Output/ApplyTransformation_Node.dream3d"
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
