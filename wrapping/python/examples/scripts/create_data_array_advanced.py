"""
Important Note
==============

This python file can be used as an example of how to execute the Create Data Array
(Advanced) filter using all the various initialization options.

If you plan to use the codes below (and you are welcome to), there are a few things
that you, the developer, should take note of:

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

# Create a Data Structure
data_structure = nx.DataStructure()

#------------------------------------------------------------------------------
# Create Data Array With Fill Value
#------------------------------------------------------------------------------
output_array_path = nx.DataPath(["Fill Value Array"])
array_type = nx.NumericType.float32
tuple_dims = [[10]]
component_dims = [[3, 2]]
create_array_nx_filter = nx.CreateDataArrayAdvancedFilter()
result  = create_array_nx_filter.execute(data_structure=data_structure, 
                                         component_dimensions=component_dims, 
                                         data_format="",
                                         init_type_index=0, # Fill value initialization type
                                         init_value="1;4;7;2;1;5", 
                                         numeric_type_index=array_type, 
                                         output_array_path=output_array_path, 
                                         tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArrayFilter, result)


# We can check the output of the filter by simply printing the array
npdata = data_structure[output_array_path].npview()
print(npdata)

#------------------------------------------------------------------------------
# Create Data Array With Incremental Values
#------------------------------------------------------------------------------
output_array_path = nx.DataPath(["Incremental Value Array"])
array_type = nx.NumericType.int16
tuple_dims = [[3, 3]]
component_dims = [[3]]
create_array_nx_filter = nx.CreateDataArrayAdvancedFilter()
result  = create_array_nx_filter.execute(data_structure=data_structure, 
                                         component_dimensions=component_dims, 
                                         data_format="",
                                         init_type_index=1, # Initialization type = Incremental/Decremental
                                         starting_fill_value="1;4;7",
                                         step_operation_index=0,  # Step type = Incrementing
                                         step_value="1;2;3",
                                         numeric_type_index=array_type, 
                                         output_array_path=output_array_path, 
                                         tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArrayFilter, result)


# We can check the output of the filter by simply printing the array
npdata = data_structure[output_array_path].npview()
print(npdata)

#------------------------------------------------------------------------------
# Create Data Array With Decremental Values
#------------------------------------------------------------------------------
output_array_path = nx.DataPath(["Decremental Value Array"])
array_type = nx.NumericType.int32
tuple_dims = [[3, 3]]
component_dims = [[3]]
create_array_nx_filter = nx.CreateDataArrayAdvancedFilter()
result  = create_array_nx_filter.execute(data_structure=data_structure, 
                                         component_dimensions=component_dims, 
                                         data_format="",
                                         init_type_index=1, # Initialization type = Incremental/Decremental
                                         starting_fill_value="1;4;7",
                                         step_operation_index=1,  # Step type = Decrementing
                                         step_value="1;2;3",
                                         numeric_type_index=array_type, 
                                         output_array_path=output_array_path, 
                                         tuple_dimensions=tuple_dims)
nxtest.check_filter_result(nx.CreateDataArrayFilter, result)


# We can check the output of the filter by simply printing the array
npdata = data_structure[output_array_path].npview()
print(npdata)

#------------------------------------------------------------------------------
# Create Data Array With Random Values
#------------------------------------------------------------------------------
output_array_path = nx.DataPath(["Random Value Array"])
array_type = nx.NumericType.int8
tuple_dims = [[4, 4]]
component_dims = [[2]]
create_array_nx_filter = nx.CreateDataArrayAdvancedFilter()
result  = create_array_nx_filter.execute(data_structure=data_structure, 
                                         component_dimensions=component_dims, 
                                         data_format="",
                                         init_type_index=2, # Initialization type = Random
                                         numeric_type_index=array_type, 
                                         output_array_path=output_array_path, 
                                         tuple_dimensions=tuple_dims,
                                         seed_array_name="Seed Value Array")
nxtest.check_filter_result(nx.CreateDataArrayFilter, result)


# We can check the output of the filter by simply printing the array
npdata = data_structure[output_array_path].npview()
print(npdata)

#------------------------------------------------------------------------------
# Create Data Array With Random Values In A Range
#------------------------------------------------------------------------------
output_array_path = nx.DataPath(["Random Values In A Range Array"])
array_type = nx.NumericType.int8
tuple_dims = [[4, 4]]
component_dims = [[2]]
create_array_nx_filter = nx.CreateDataArrayAdvancedFilter()
result  = create_array_nx_filter.execute(data_structure=data_structure, 
                                         component_dimensions=component_dims, 
                                         data_format="",
                                         init_type_index=3, # Initialization type = Random With Range
                                         init_start_range="-2;4",
                                         init_end_range="4;7",
                                         numeric_type_index=array_type, 
                                         output_array_path=output_array_path, 
                                         tuple_dimensions=tuple_dims,
                                         seed_array_name="Seed Value Array 2")
nxtest.check_filter_result(nx.CreateDataArrayFilter, result)


# We can check the output of the filter by simply printing the array
npdata = data_structure[output_array_path].npview()
print(npdata)