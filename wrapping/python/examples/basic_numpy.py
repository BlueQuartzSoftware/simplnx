import complex as cx
import numpy as np


#------------------------------------------------------------------------------
# Notes:
# As of conda complex version 1.0.0 there is *NO* way to wrap an existing
# numpy array. You will have to make a copy of the data into a complex DataArray
# or have complex create the DataArray for you and load your data into the
# DataArray (Overwriting the initialization values).
#
# This will hopefully be addressed in a future update.
#------------------------------------------------------------------------------
# Create a Data Structure
data_structure = cx.DataStructure()


array_path = cx.DataPath(['data'])
assert cx.CreateDataArray.execute(data_structure, 
                                  numeric_type=cx.NumericType.float32, 
                                  component_count=1, 
                                  tuple_dimensions=[[3, 2]], 
                                  output_data_array=array_path, 
                                  initialization_value='0')

npdata = data_structure[array_path].npview()
# Manipulate the underlying array
npdata += 42.0

# Create a copy of the underlying array into a new Numpy array
degrees_data = npdata.copy()

# Use Numpy to convert the data to radians
radians_data = np.radians(degrees_data)

# Run a D3D filter to convert back to degrees
assert cx.ChangeAngleRepresentation.execute(data_structure, conversion_type=0, angles_array_path=array_path)

# compare the 2 arrays
assert np.array_equal(npdata, radians_data)

# Print the 2 data arrays 
print('complex:')
print(npdata)
print('numpy:')
print(radians_data)
