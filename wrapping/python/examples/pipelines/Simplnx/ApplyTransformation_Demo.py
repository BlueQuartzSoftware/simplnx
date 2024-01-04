import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = nx.CreateDataGroup()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    data_object_path=nx.DataPath("Group 1")
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.CreateDataGroup()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    data_object_path=nx.DataPath("Group 1/Group 2")
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.CreateGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_handling=0,
    cell_attribute_matrix_name="Cell Data",
    dimensions=[10, 10 , 2],
    #edge_attribute_matrix_name="Edge Data", (not used here)
    #edge_list_name: DataPath = ..., (not used here)
    #face_attribute_matrix_name="Face Data", (not used here)
    geometry_name=nx.DataPath("Group 1/Group 2/[Geometry]"),
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
nxtest.check_filter_result(filter, result)


# Filter 4
# Instantiate Filter
filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    component_count=1,
    data_format="",
    initialization_value="2",
    numeric_type=nx.NumericType.int32,
    output_data_array=nx.DataPath("Group 1/Group 2/[Geometry]/Cell Data/Data"),
    tuple_dimensions=[[2.0, 10.0, 10.0]]
)
nxtest.check_filter_result(filter, result)

# Filter 5
# Instantiate Filter
filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Group 1/Group 2/[Geometry]/Cell Data"),
    interpolation_type=1,
    scale=[2.0, 2.0, 2.0],
    selected_image_geometry=nx.DataPath("Group 1/Group 2/[Geometry]"),
    transformation_type=5,
    translate_geometry_to_global_origin=False
)
nxtest.check_filter_result(filter, result)


print("===> Pipeline Complete")