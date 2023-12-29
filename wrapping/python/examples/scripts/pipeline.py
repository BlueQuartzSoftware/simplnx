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

data_structure = nx.DataStructure()

pipeline = nx.Pipeline().from_file(nxtest.GetSimplnxSourceDir() + '/src/Plugins/OrientationAnalysis/pipelines/EBSD Reconstruction/(01) Small IN100 Archive.d3dpipeline')

pipeline.to_file( "test pipeline", nxtest.GetTestTempDirectory() + "/python_pipeline.d3dpipeline")



# pipeline.append(nx.CreateDataArray(), {'numeric_type': nx.NumericType.int32})
# pipeline[0].set_args({'numeric_type': nx.NumericType.int32})

# did_execute = pipeline.execute(data_structure)

# print('Pipeline Execute: {}'.format(did_execute))


