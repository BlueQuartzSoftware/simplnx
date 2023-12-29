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

import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = nxtest.GetTestTempDirectory() + "/basic_ebsd_example.dream3d"
import_data.data_paths = None  # Use 'None' to import the entire file.

print(f'{import_data.file_path}')

result = nx.ReadDREAM3DFilter.execute(data_structure=data_structure, import_file_data=import_data)
nxtest.check_filter_result(nx.ReadDREAM3DFilter, result)

#------------------------------------------------------------------------------
# Print out the children of some of the Attribute Matrix groups
#------------------------------------------------------------------------------
children = data_structure.get_children("Small IN100")
for child in children:
    print(f'{child}')

children = data_structure.get_children("Small IN100/Scan Data")
for child in children:
    print(f'{child}')

#------------------------------------------------------------------------------
# Get the underlying data from the DataStructure
#------------------------------------------------------------------------------
npview_data_path = nx.DataPath("Small IN100/Scan Data/Image Quality")
npview = data_structure[npview_data_path].npview()

# Change the underlying data based on some criteria using Numpy
npview[npview < 120] = 0

#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------
output_file_path = nxtest.GetTestTempDirectory() + "/import_data.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(nx.WriteDREAM3DFilter, result)

