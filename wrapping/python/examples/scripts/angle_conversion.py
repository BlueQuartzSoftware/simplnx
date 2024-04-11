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

import itkimageprocessing as nxitk
import orientationanalysis as nxor
import simplnx_test_dirs as nxtest

import numpy as np

#------------------------------------------------------------------------------
# Print the various filesystem paths that are pregenerated for this machine.
#------------------------------------------------------------------------------
nxtest.print_all_paths()


# ------------------------------------------------------------------------------
# Create a DataArray that is as long as my CSV file (99 Rows in this case)
# ------------------------------------------------------------------------------
# Create a Data Structure
data_structure = nx.DataStructure()
# Create a DataArray to copy the Euler Angles into 
array_path = nx.DataPath(['Euler Angles'])
result = nx.CreateDataArray.execute(data_structure=data_structure,
                                    numeric_type=nx.NumericType.float32,
                                    component_count=3,
                                    tuple_dimensions=[[99]],
                                    output_array_path=array_path,
                                    initialization_value_str='0')
nxtest.check_filter_result(nx.CreateDataArray, result)


# Get a numpy.view into the newly created DataArray
data_array = data_structure[array_path]
# Get the underlying DataStore object
data_store = data_array.store
# Get a Numpy View into the data
npdata = data_store.npview()

# Read the CSV file into the DataArray using the numpy view
file_path = nxtest.get_simplnx_python_source_dir() / 'examples/data/angles.csv'
npdata[:] = np.loadtxt(file_path, delimiter=',')

# ------------------------------------------------------------------------------
# Run the ConvertOrientation Filter to convert the Eulers to Quaternions
# ------------------------------------------------------------------------------
quat_path = nx.DataPath(['Quaternions'])
result = nxor.ConvertOrientations.execute(data_structure=data_structure,
                                          input_orientation_array_path=array_path,
                                          input_type=0,
                                          output_orientation_array_name='Quaternions',
                                          output_type=2)
nxtest.check_filter_result(nxor.ConvertOrientations, result)


# Get the Quaternions and print them out.
data_array = data_structure[quat_path]
data_store = data_array.store
npdata = data_store.npview()

print(npdata)

