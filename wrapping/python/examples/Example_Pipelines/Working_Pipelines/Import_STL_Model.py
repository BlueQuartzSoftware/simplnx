import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.StlFileReaderFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    stl_file_path="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/STL_Models/Cylinder.stl",
    triangle_geometry_name=cx.DataPath("[Triangle Geometry]"),
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
    triangle_areas_array_path="Areas",
    triangle_geometry_data_path=cx.DataPath("[Triangle Geometry]")
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
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="0",
    numeric_type=cx.NumericType.float32,
    output_data_array=cx.DataPath("Node Type"),
    tuple_dimensions=[[1.0]]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")