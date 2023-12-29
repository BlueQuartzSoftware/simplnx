import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = nx.ReadStlFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    stl_file_path=nxtest.GetDataDirectory() + "/STL_Models/Cylinder.stl",
    triangle_geometry_name=nx.DataPath("[Triangle Geometry]"),
    vertex_attribute_matrix="Vertex Data"
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.CalculateTriangleAreasFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    triangle_areas_array_path="Areas",
    triangle_geometry_data_path=nx.DataPath("[Triangle Geometry]")
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="0",
    numeric_type=nx.NumericType.float32,
    output_data_array=nx.DataPath("Node Type"),
    tuple_dimensions=[[1.0]]
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")