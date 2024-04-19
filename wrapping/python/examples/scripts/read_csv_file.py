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

# Create the DataStructure object
data_structure = nx.DataStructure()
# This file has 7 columns to import
read_csv_data = nx.ReadCSVDataParameter()
read_csv_data.input_file_path = str(nxtest.get_simplnx_python_source_dir() / "examples/data/test_csv_data.csv")
read_csv_data.start_import_row = 2
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = []
read_csv_data.column_data_types = [nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.float32,nx.DataType.int32 ]
read_csv_data.skipped_array_mask = [False,False,False,False,False,False,False ]
read_csv_data.tuple_dims = [37989]
read_csv_data.headers_line = 1
read_csv_data.header_mode = nx.ReadCSVDataParameter.HeaderMode.Line

# This will store the imported arrays into a newly generated DataGroup
result = nx.ReadCSVFileFilter.execute(data_structure=data_structure, 
                                      # This will store the imported arrays into a newly generated DataGroup
                                      created_data_group_path=nx.DataPath(["Imported Data"]),  
                                      # We are not using this parameter but it still needs a value
                                      selected_attribute_matrix_path=nx.DataPath(),  
                                      # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                                      use_existing_group=False,   
                                      # The ReadCSVData object with all member variables set.
                                      read_csv_data_object=read_csv_data # The ReadCSVData object with all member variables set.
                                      )
nxtest.check_filter_result(nx.ReadCSVFileFilter, result)

