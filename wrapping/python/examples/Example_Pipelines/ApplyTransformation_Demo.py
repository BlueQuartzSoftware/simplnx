import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.CreateDataGroup()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    data_object_path=cx.DataPath("Group 1")
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
filter = cx.CreateDataGroup()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    data_object_path=cx.DataPath("Group 2")
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
filter = cx.CreateGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_handling=0,
    cell_attribute_matrix_name="Cell Data",
    dimensions=[10, 10 , 2],
    #edge_attribute_matrix_name="Edge Data", (not used here)
    #edge_list_name: DataPath = ..., (not used here)
    #face_attribute_matrix_name="Face Data", (not used here)
    geometry_name=cx.DataPath("[Geometry]"), 
    geometry_type=0,
    #hexahedral_list_name: DataPath = ..., (not used here)
    length_unit_type=7,
    origin=[0.0, 0.0, 0.0],
    #quadrilateral_list_name: DataPath = ..., (not used here)
    spacing=[1.0, 1.0, 1.0],
    #tetrahedral_list_name: DataPath = ..., (not used here)
    #triangle_list_name: DataPath = ..., (not used here)
    #vertex_attribute_matrix_name="Vertex Data", (not used here)
    #vertex_list_name: DataPath = ..., (not used here)
    warnings_as_errors=False
    #x_bounds: DataPath = ..., (not used here)
    #y_bounds: DataPath = ..., (not used here)
    #z_bounds: DataPath = ...  (not used here)
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the CreateGeometryFilter")


# Filter 4
# Instantiate Filter
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    component_count=1,
    data_format="Unknown",
    initialization_value="2",
    numeric_type=4.0,
    output_data_array=cx.DataPath("Data"),
    tuple_dimensions=[0.0, 2.0, 10.0, 10.0]
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
    cell_attribute_matrix_path=cx.DataPath("Group 1/Group 2/[Geometry]/Cell Data"),
    interpolation_type=1,
    scale=[2.0, 2.0, 2.0],
    selected_image_geometry=cx.DataPath("Group 1/Group 2/[Geometry]/Cell Data"),
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