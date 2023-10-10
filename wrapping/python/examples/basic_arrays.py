# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

#------------------------------------------------------------------------------
# Create a top level group: (Not needed)
#------------------------------------------------------------------------------
result = cx.CreateDataGroup.execute(data_structure=data_structure,
                                    data_object_path=cx.DataPath(['Group']))
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running CreateDataGroup filter")

result = cx.CreateDataGroup.execute(data_structure=data_structure, 
                                    data_object_path=cx.DataPath("/Some/Path/To/Group"));
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running CreateDataGroup filter")

#------------------------------------------------------------------------------
# Create 1D Array 
#------------------------------------------------------------------------------
# Create the Input Parameters to the CreateDataArray filter
output_array_path = cx.DataPath(["Group", "1D Array"])
array_type = cx.NumericType.float32
tuple_dims = [[10]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, 
                                      component_count=1, 
                                      data_format="", 
                                      initialization_value="10", 
                                        numeric_type=array_type, 
                                        output_data_array=output_array_path, 
                                        tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

# We can check the output of the filter by simply printing the array
npdata = data_structure[output_array_path].npview()
print(npdata)

#------------------------------------------------------------------------------
# Create 2D Array 
#------------------------------------------------------------------------------

# Create a 2D Array with dimensions 2, 5 where the 5 is the fastest moving dimension.
# Example, and Image where 5 wide x 2 High
output_array_path = cx.DataPath(["2D Array"])
tuple_dims = [[2,5]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, 
                                      component_count=1, 
                                      data_format="", 
                                      initialization_value="10", 
                                        numeric_type=array_type, 
                                        output_data_array=output_array_path, 
                                        tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

npdata = data_structure[output_array_path].npview()
print(npdata)


#------------------------------------------------------------------------------
# Create 3D Array 
#------------------------------------------------------------------------------

# Create a 3D Array with dimensions 3, 2, 5 where the 5 is the fastest moving dimension.
# Example, and Image where 5 wide x 2 High
output_array_path = cx.DataPath(["3D Array"])
tuple_dims = [[3, 2, 5]]
create_array_filter = cx.CreateDataArray()
result  = create_array_filter.execute(data_structure=data_structure, 
                                      component_count=1, 
                                      data_format="", 
                                      initialization_value="10", 
                                        numeric_type=array_type, 
                                        output_data_array=output_array_path, 
                                        tuple_dimensions=tuple_dims)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

npdata = data_structure[output_array_path].npview()
print(npdata)

result = cx.CreateAttributeMatrixFilter.execute(data_structure=data_structure, 
                                                data_object_path=cx.DataPath(["New Attribute Matrix"]), 
                                                tuple_dimensions = [[100., 200., 300.]])
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running CreateAttributeMatrixFilter filter")



output_file_path = "/tmp/output_file_example.dream3d"
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")