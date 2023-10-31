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
    #scale_factor: float = ...,
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/Cylinder.stl"),
    triangle_geometry_name=cx.DataPath("[Triangle Geometry]"),
    vertex_attribute_matrix="Vertex Data"
)

#Filter 2

result = cx.CalculateTriangleAreasFilter.execute(
    data_structure=data_structure,
    triangle_areas_array_path="Areas",
    triangle_geometry_data_path=cx.DataPath("[Triangle Geometry]")
)

#Filter 3

result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="Unknown",
    initialization_value="0",
    numeric_type=8,
    output_data_array=cx.DataPath("Node Type"),
    tuple_dimensions=[0.0, 1.0]
)