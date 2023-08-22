# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Let's get a data array created to have something to write.

# Create a Data Structure
data_structure = cx.DataStructure()

# Create a 2D Array with dimensions 2, 5 where the 5 is the fastest moving dimension.
# Example, and Image where 5 wide x 2 High
output_array_path = cx.DataPath(["3D Array"])
array_type = cx.NumericType.float32
tuple_dims = [[3, 2,5]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, component_count=1, data_format="", initialization_value="10", 
                            numeric_type=array_type, output_data_array=output_array_path, tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

data_array = data_structure[output_array_path]
data_store = data_array.store
npdata = data_store.npview()
print(npdata)

output_file_path = "output_file_example.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

