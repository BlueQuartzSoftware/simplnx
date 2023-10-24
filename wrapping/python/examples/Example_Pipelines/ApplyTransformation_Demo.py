import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
result = cx.CreateDataGroup.execute(
    data_structure=data_structure,
    data_object_path=cx.DataPath("Group 1"),
)
#Filter 2
result = cx.CreateDataGroup.execute(
    data_structure=data_structure,
    data_object_path=cx.DataPath("Group 2"),
)
#Filter 3
result = cx.CreateGeometryFilter.execute(
    data_structure=data_structure,
    array_handling=0,
    cell_attribute_matrix_name="Cell Data",
    dimensions=[10, 10 , 2],
    #edge_attribute_matrix_name="Edge Data",
    #edge_list_name: DataPath = ...,
    #face_attribute_matrix_name="Face Data",
    geometry_name=cx.DataPath("[Geometry]"),
    geometry_type=0,
    #hexahedral_list_name: DataPath = ...,
    length_unit_type=7,
    origin=[0.0, 0.0, 0.0],
    #quadrilateral_list_name: DataPath = ...,
    spacing=[1.0, 1.0, 1.0],
    #tetrahedral_list_name: DataPath = ...,
    #triangle_list_name: DataPath = ...,
    #vertex_attribute_matrix_name="Vertex Data",
    #vertex_list_name: DataPath = ...,
    warnings_as_errors=False,
    #x_bounds: DataPath = ...,
    #y_bounds: DataPath = ...,
    #z_bounds: DataPath = ...
)
#Filter 4
result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    #advanced_options: bool = ...,
    component_count=1,
    data_format="Unknown",
    initialization_value="2",
    numeric_type=4.0,
    output_data_array=cx.DataPath("Data"),
    tuple_dimensions=[0.0, 2.0, 10.0, 10.0],
)
#Filter 5
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Group 1/Group 2/[Geometry]/Cell Data"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=1,
    #manual_transformation_matrix: List[List[float]] = ...,
    #rotation: List[float] = ...,
    scale=[2.0, 2.0, 2.0],
    selected_image_geometry=cx.DataPath("Group 1/Group 2/[Geometry]/Cell Data"),
    transformation_type=5,
    translate_geometry_to_global_origin=False
    #translation: List[float] = ...
)