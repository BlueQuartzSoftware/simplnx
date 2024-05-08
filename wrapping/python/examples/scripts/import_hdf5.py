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

# Create the ReadH5EbsdFileParameter and assign values to it.
h5ebsdParameter = nxor.ReadH5EbsdFileParameter.ValueType()
h5ebsdParameter.euler_representation=0
h5ebsdParameter.end_slice=117
h5ebsdParameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
h5ebsdParameter.input_file_path="Data/Output/Reconstruction/Small_IN100.h5ebsd"
h5ebsdParameter.start_slice=1
h5ebsdParameter.use_recommended_transform=True

# Execute Filter with Parameters
result = nxor.ReadH5EbsdFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    output_image_geometry_path=nx.DataPath("DataContainer"),
    read_h5_ebsd_object=h5ebsdParameter
)

dataset1 = nx.ReadHDF5DatasetParameter.DatasetImportInfo()
dataset1.dataset_path = "/DataStructure/Small IN100/Scan Data/Confidence Index"
dataset1.tuple_dims = "1,201,189"
dataset1.component_dims = "1"

dataset2 = nx.ReadHDF5DatasetParameter.DatasetImportInfo()
dataset2.dataset_path = "/DataStructure/Small IN100/Scan Data/EulerAngles"
dataset2.tuple_dims = "1,201,189"
dataset2.component_dims = "3"

import_hdf5_param = nx.ReadHDF5DatasetParameter.ValueType()
import_hdf5_param.input_file = str(nxtest.get_test_temp_directory() / "basic_ebsd_example.dream3d")
import_hdf5_param.datasets = [dataset1, dataset2]
# import_hdf5_param.parent = nx.DataPath(["Imported Data"])

result = nx.ReadHDF5DatasetFilter.execute(data_structure=data_structure,
                                          import_hdf5_object=import_hdf5_param
                                      )
nxtest.check_filter_result(nx.ReadHDF5DatasetFilter, result)

