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

# Let's get a data array created to have something to write.

# Create a Data Structure
data_structure = nx.DataStructure()

# Create a 2D Array with dimensions 2, 5 where the 5 is the fastest moving dimension.
# Example, and Image where 5 wide x 2 High
output_array_path = nx.DataPath(["3D Array"])
array_type = nx.NumericType.float32
tuple_dims = [[3, 2,5]]
create_array_nx_filter = nx.CreateDataArrayFilter()
result  = create_array_nx_filter.execute(data_structure=data_structure, component_count=1, data_format="", initialization_value_str="10", 
                            numeric_type_index=array_type, output_array_path=output_array_path, tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArrayFilter, result)


npdata = data_structure[output_array_path].npview()
print(npdata)

output_file_path = nxtest.get_test_temp_directory() / "output_file_example.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(nx.WriteDREAM3DFilter, result)

