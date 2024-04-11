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

#------------------------------------------------------------------------------
# Create a Data Structure
#------------------------------------------------------------------------------
data_structure = nx.DataStructure()

# Import an EBSD Data file
result = nxor.ReadAngDataFilter.execute(data_structure=data_structure, 
                               cell_attribute_matrix_name="Scan Data", 
                               cell_ensemble_attribute_matrix_name="Phase Data",
                               output_image_geometry_path=nx.DataPath(["Small IN100"]), 
                               input_file=nxtest.get_data_directory() / "Small_IN100/Slice_1.ang")

nxtest.check_filter_result(nxor.ReadAngDataFilter, result)

#------------------------------------------------------------------------------
# Rotate the Euler Reference Frame
#------------------------------------------------------------------------------
result = nxor.RotateEulerRefFrameFilter.execute(data_structure=data_structure, 
                                                euler_angles_array_path=nx.DataPath(["Small IN100", "Scan Data", "EulerAngles"]), 
                                                rotation_axis=[0,0,1,90])
nxtest.check_filter_result(nxor.RotateEulerRefFrameFilter, result)

#------------------------------------------------------------------------------
# Rotate the Sample Reference Frame 180@010
#------------------------------------------------------------------------------
result = nx.RotateSampleRefFrameFilter.execute(data_structure=data_structure,
    # output_image_geometry_path=nx.DataPath(["Small IN100 Rotated"]),
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0,1,0,180],
    rotation_representation=0,
    input_image_geometry_path=nx.DataPath(["Small IN100"]),
    #rotation_matrix=[[1,0,0],[0,1,0],[0,0,1]]
    )
nxtest.check_filter_result(nx.RotateSampleRefFrameFilter, result)


#------------------------------------------------------------------------------
# Create a ThresholdSet to use in the MultiThreshold Objects filter
# This will create a boolean output array at DataPath(["Small IN100", "Scan Data", "Mask"]
#------------------------------------------------------------------------------
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = nx.ArrayThreshold()
threshold_2.array_path = nx.DataPath(["Small IN100", "Scan Data", "Image Quality"])
threshold_2.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = nx.DataType.boolean
result = nx.MultiThresholdObjects.execute(data_structure=data_structure,
                                        array_thresholds_object=threshold_set, 
                                        output_data_array_name="Mask",
                                        created_mask_type=nx.DataType.boolean)
nxtest.check_filter_result(nx.MultiThresholdObjects, result)


#------------------------------------------------------------------------------
# Generate the IPF Colors for the <001> direction
#------------------------------------------------------------------------------
result = nxor.GenerateIPFColorsFilter.execute(    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath(["Small IN100", "Scan Data", "EulerAngles"]),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=nx.DataPath(["Small IN100", "Scan Data", "Phases"]),
    crystal_structures_array_path=nx.DataPath(["Small IN100", "Phase Data", "CrystalStructures"]),
    mask_array_path=nx.DataPath(["Small IN100", "Scan Data", "Mask"]),
    reference_dir=[0,0,1],
    use_mask=True
)
nxtest.check_filter_result(nxor.GenerateIPFColorsFilter, result)



#------------------------------------------------------------------------------
# Write the IPF colors to a PNG file
#------------------------------------------------------------------------------
result = nxitk.ITKImageWriter.execute(data_structure=data_structure, 
                                      file_name=nxtest.get_test_temp_directory() / "Small_IN100_IPF_Z.png", 
                                      image_array_path=nx.DataPath(["Small IN100", "Scan Data", "IPFColors"]),
                                      input_image_geometry_path=nx.DataPath(["Small IN100"]),
                                      index_offset=0,
                                      plane=0)
nxtest.check_filter_result(nxitk.ITKImageWriter, result)

# #------------------------------------------------------------------------------
# # Show the IPFColors using MatPlotLib
# #------------------------------------------------------------------------------
# # First get the array from the DataStructure
# data_array = data_structure[nx.DataPath(["Small IN100", "Scan Data", "IPFColors"])]
# # Get the underlying DataStore object
# data_store = data_array.store
# npdata = data_store.npview().copy()
# # Remove any dimension with '1'
# npdata = np.squeeze(npdata, axis=0)

# plt.imshow(npdata)
# plt.title("Small IN100 IPF [001]")
# plt.axis('off')  # to turn off axes
# plt.show()


#------------------------------------------------------------------------------
# Generate a Colorized Version of the Confidence Index
#------------------------------------------------------------------------------
result = nx.GenerateColorTableFilter.execute(data_structure=data_structure,
                                              output_rgb_array_name="CI Color", 
                                              input_data_array_path=nx.DataPath(["Small IN100", "Scan Data", "Confidence Index"]), 
                                              selected_preset="Rainbow Desaturated")
nxtest.check_filter_result(nx.GenerateColorTableFilter, result)


#------------------------------------------------------------------------------
# Create a Pole Figure
#------------------------------------------------------------------------------
prefix = "Small_IN100_"
result = nxor.WritePoleFigureFilter.execute(data_structure=data_structure,
                                            cell_euler_angles_array_path=nx.DataPath(["Small IN100", "Scan Data", "EulerAngles"]), 
                                            cell_phases_array_path=nx.DataPath(["Small IN100", "Scan Data", "Phases"]),
                                            crystal_structures_array_path=nx.DataPath(["Small IN100", "Phase Data", "CrystalStructures"]),
                                            generation_algorithm=1, # Discrete = 1 
                                            mask_array_path=nx.DataPath(["Small IN100", "Scan Data", "Mask"]), 
                                            output_image_geometry_path=nx.DataPath(["Small IN100 Pole Figure"]), 
                                            image_layout=0, # O = Horizontal Layout 
                                            image_prefix=prefix, 
                                            image_size=512, 
                                            lambert_size=64, 
                                            material_name_array_path=nx.DataPath(["Small IN100", "Phase Data", "MaterialName"]), 
                                            num_colors=32, 
                                            output_path=nxtest.get_test_temp_directory() / "small_in100_pole_figure", 
                                            save_as_image_geometry=True, 
                                            title="Small IN100 Slice 1", 
                                            use_mask=True, 
                                            write_image_to_disk=True)
nxtest.check_filter_result(nxor.WritePoleFigureFilter, result)



#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------

output_file_path = nxtest.get_test_temp_directory() / "basic_ebsd_example.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(nx.WriteDREAM3DFilter, result)

