import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = nx.CreateDataGroupFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    data_object_path=nx.DataPath("Group 1")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.CreateDataGroupFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    data_object_path=nx.DataPath("Group 1/Group 2")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.CreateGeometryFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_handling_index=0,
    cell_attribute_matrix_name="Cell Data",
    dimensions=[10, 10 , 2],
    #edge_attribute_matrix_name="Edge Data", (not used here)
    #edge_list_name: DataPath = ..., (not used here)
    #face_attribute_matrix_name="Face Data", (not used here)
    output_geometry_path=nx.DataPath("Group 1/Group 2/[Geometry]"),
    geometry_type_index=0,
    #hexahedral_list_name: DataPath = ..., (not used here)
    length_unit_index=7,
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
nxtest.check_filter_result(nx_filter, result)


# Filter 4
# Instantiate Filter
nx_filter = nx.CreateDataArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    component_dimensions=[[1]],
    data_format="",
    init_value="2",
    numeric_type_index=nx.NumericType.int32,
    output_array_path=nx.DataPath("Group 1/Group 2/[Geometry]/Cell Data/Data"),
    tuple_dimensions=[[2.0, 10.0, 10.0]]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Group 1/Group 2/[Geometry]/Cell Data"),
    interpolation_type_index=1,
    scale=[2.0, 2.0, 2.0],
    input_image_geometry_path=nx.DataPath("Group 1/Group 2/[Geometry]"),
    transformation_type_index=5,
    translate_geometry_to_global_origin=False
)
nxtest.check_filter_result(nx_filter, result)


print("===> Pipeline Complete")