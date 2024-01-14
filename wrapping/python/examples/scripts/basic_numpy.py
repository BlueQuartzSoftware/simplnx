"""
Important Note
==============

This python file can be used as an example of how to execute a number of DREAM3D-NX
filters one after another, if you plan to use the codes below (and you are welcome to),
there are a few things that you, the developer, should take note of:

Import Statements
-----------------

You will most likely *NOT* need to include the following code:

   .. code:: python
      
      import simplnx_test_dirs as nxtest

Filter Error Detection
----------------------

In each section of code a filter is created and executed immediately. This may or
may *not* be what you want to do. You can also preflight the filter to verify the
correctness of the filters before executing the filter **although** this is done
for you when the filter is executed. As such, you will want to check the 'result'
variable to see if there are any errors or warnings. If there **are** any then
you, as the developer, should act appropriately on the errors or warnings. 
More specifically, this bit of code:

   .. code:: python

      nxtest.check_filter_result(nxor.ReadAngDataFilter, result)

is used by the simplnx unit testing framework and should be replaced by your own
error checking code. You are welcome to look up the function definition and use
that.

"""
import simplnx as nx
import simplnx_test_dirs as nxtest


import numpy as np


#------------------------------------------------------------------------------
# Notes:
# As of conda simplnx version 1.0.0 there is *NO* way to wrap an existing
# numpy array. You will have to make a copy of the data into a simplnx DataArray
# or have simplnx create the DataArray for you and load your data into the
# DataArray (Overwriting the initialization values).
#
# This will hopefully be addressed in a future update.
#------------------------------------------------------------------------------
# Create a Data Structure
data_structure = nx.DataStructure()


array_path = nx.DataPath(['data'])
assert nx.CreateDataArray.execute(data_structure, 
                                  numeric_type=nx.NumericType.float32, 
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
result = nx.ChangeAngleRepresentation.execute(data_structure, conversion_type=0, angles_array_path=array_path)
nxtest.check_filter_result(nx.ChangeAngleRepresentation, result)

# compare the 2 arrays
assert np.array_equal(npdata, radians_data)

# Print the 2 data arrays 
print('simplnx:')
print(npdata)
print('numpy:')
print(radians_data)

