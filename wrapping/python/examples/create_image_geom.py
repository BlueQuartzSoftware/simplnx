import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create the DataStructure object and define the dimensions of the geometry
data_structure = cx.DataStructure()
ig_dims = [10, 20, 30] # NOTE: These are in XYZ order
result = cx.CreateGeometryFilter.execute(  data_structure=data_structure,
    array_handling= 0,  # This does not matter for Image Geometry
    cell_attribute_matrix_name="Cell Data",
    dimensions=ig_dims, # Note that the dimensions are list as  X, Y, Z
    geometry_name=cx.DataPath(["Image Geometry"]),
    geometry_type=0, # 0 = Image Geometry
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0])
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateGeometryFilter filter")


# Now we can create some (or import from another source) some cell based data
# this is data that lives at the center of each cell
# NOTE: we do *not* need to set the tuple dimensions because we are adding this array to the 
# AttributeMatrix that we generated in the last filter.
output_array_path = cx.DataPath(["Image Geometry", "Cell Data", "Float Cell Data"])
array_type = cx.NumericType.float32
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, component_count=1, data_format="", initialization_value="10", 
                            numeric_type=array_type, output_data_array=output_array_path)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the CreateDataArray filter")


output_file_path = "/tmp/output.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, export_file_path=output_file_path, write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ExportDREAM3DFilter filter")
