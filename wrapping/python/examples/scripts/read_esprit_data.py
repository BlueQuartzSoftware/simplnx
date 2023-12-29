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

param1 = nxor.OEMEbsdScanSelectionParameter.ValueType()
param1.input_file_path = "LEROY_0089_Section_382.h5"
param1.stacking_order = 0
param1.scan_names = ["LEROY_0089_Section_382"]

result = nxor.ReadH5EspritDataFilter.execute(data_structure = data_structure, 
                                             cell_attribute_matrix_name = "Cell Data", 
                                             cell_ensemble_attribute_matrix_name = "Cell Ensemble Data", 
                                             degrees_to_radians = True, 
                                             image_geometry_name = nx.DataPath("ImageGeom"),
                                             origin = [0.0, 0.0, 0.0], 
                                             read_pattern_data = False, 
                                             selected_scan_names = param1, 
                                             z_spacing = 1.0)

nxtest.check_filter_result(nxor.ReadH5EspritDataFilter, result)

#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------
output_file_path = nxtest.GetTestTempDirectory() + "/import_esprit.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(nx.WriteDREAM3DFilter, result)
