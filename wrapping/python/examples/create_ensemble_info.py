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
      
      import complex_test_dirs as cxtest

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

      cxtest.check_filter_result(cxor.ReadAngDataFilter, result)

is used by the simplnx unit testing framework and should be replaced by your own
error checking code. You are welcome to look up the function definition and use
that.

"""
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

#------------------------------------------------------------------------------
# Print the various filesystem paths that are pregenerated for this machine.
#------------------------------------------------------------------------------
cxtest.print_all_paths()

# Create a Data Structure
data_structure = nx.DataStructure()

ensemble_info_parameter = []
ensemble_info_parameter.append(["Hexagonal-High 6/mmm","Primary","Phase 1"])
ensemble_info_parameter.append(["Cubic-High m-3m","Primary","Phase 2"])

create_ensemble_info = cxor.CreateEnsembleInfoFilter()
result = create_ensemble_info.execute(data_structure=data_structure,
                             cell_ensemble_attribute_matrix_name=nx.DataPath(["Phase Information"]), 
                             crystal_structures_array_name="CrystalStructures", 
                             phase_names_array_name="Phase Names", 
                             phase_types_array_name="Primary", 
                             ensemble=ensemble_info_parameter
                             )
cxtest.check_filter_result(cxor.CreateEnsembleInfoFilter, result)
