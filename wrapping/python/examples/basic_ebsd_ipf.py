# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np


#------------------------------------------------------------------------------
# Create a Data Structure
#------------------------------------------------------------------------------
data_structure = cx.DataStructure()

# Import an EBSD Data file
result = cxor.ReadAngDataFilter.execute(data_structure=data_structure, 
                               cell_attribute_matrix_name="Scan Data", 
                               cell_ensemble_attribute_matrix_name="Phase Data",
                               data_container_name=cx.DataPath(["Small IN100"]), 
                               input_file=cxtest.GetDataDirectory() + "/Data/SmallIN100/Slice_1.ang")
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ReadAngDataFilter")

#------------------------------------------------------------------------------
# Rotate the Euler Reference Frame
#------------------------------------------------------------------------------
result = cxor.RotateEulerRefFrameFilter.execute(data_structure=data_structure, 
                                                euler_angles_array_path=cx.DataPath(["Small IN100", "Scan Data", "EulerAngles"]), 
                                                rotation_axis=[0,0,1,90])
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the RotateEulerRefFrameFilter")


#------------------------------------------------------------------------------
# Rotate the Sample Reference Frame 180@010
#------------------------------------------------------------------------------
result = cx.RotateSampleRefFrameFilter.execute(data_structure=data_structure,
    # created_image_geometry=cx.DataPath(["Small IN100 Rotated"]),
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0,1,0,180],
    rotation_representation=0,
    selected_image_geometry=cx.DataPath(["Small IN100"]),
    #rotation_matrix=[[1,0,0],[0,1,0],[0,0,1]]
    )
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the RotateEulerRefFrameFilter")

#------------------------------------------------------------------------------
# Create a ThresholdSet to use in the MultiThreshold Objects filter
# This will create a boolean output array at DataPath(["Small IN100", "Scan Data", "Mask"]
#------------------------------------------------------------------------------
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath(["Small IN100", "Scan Data", "Image Quality"])
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = cx.DataType.boolean
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                        array_thresholds=threshold_set, 
                                        created_data_path="Mask",
                                        created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the MultiThresholdObjects")

#------------------------------------------------------------------------------
# Generate the IPF Colors for the <001> direction
#------------------------------------------------------------------------------
result = cxor.GenerateIPFColorsFilter.execute(    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath(["Small IN100", "Scan Data", "EulerAngles"]),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=cx.DataPath(["Small IN100", "Scan Data", "Phases"]),
    crystal_structures_array_path=cx.DataPath(["Small IN100", "Phase Data", "CrystalStructures"]),
    mask_array_path=cx.DataPath(["Small IN100", "Scan Data", "Mask"]),
    reference_dir=[0,0,1],
    use_mask=True
)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the GenerateIPFColorsFilter")


#------------------------------------------------------------------------------
# Write the IPF colors to a PNG file
#------------------------------------------------------------------------------
result = cxitk.ITKImageWriter.execute(data_structure=data_structure, file_name=cxtest.GetTestTempDirectory() + "/Small_IN100_IPF_Z.png", 
                                      image_array_path=cx.DataPath(["Small IN100", "Scan Data", "IPFColors"]),
                                      image_geom_path=cx.DataPath(["Small IN100"]),
                                      index_offset=0,
                                      plane=0)

if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ITKImageWriter")


# #------------------------------------------------------------------------------
# # Show the IPFColors using MatPlotLib
# #------------------------------------------------------------------------------
# # First get the array from the DataStructure
# data_array = data_structure[cx.DataPath(["Small IN100", "Scan Data", "IPFColors"])]
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
color_control_points = cx.Json('{"RGBPoints": [0,0,0,0,0.4,0.901960784314,0,0,0.8,0.901960784314,0.901960784314,0,1,1,1,1]}')
result = cx.GenerateColorTableFilter.execute(data_structure=data_structure,
                                              output_rgb_array_name="CI Color", 
                                              selected_data_array_path=cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"]), 
                                              selected_preset=color_control_points)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the GenerateColorTableFilter")

#------------------------------------------------------------------------------
# Create a Pole Figure
#------------------------------------------------------------------------------
prefix = "Small_IN100_"
result = cxor.WritePoleFigureFilter.execute(data_structure=data_structure,
                                            cell_euler_angles_array_path=cx.DataPath(["Small IN100", "Scan Data", "EulerAngles"]), 
                                            cell_phases_array_path=cx.DataPath(["Small IN100", "Scan Data", "Phases"]),
                                            crystal_structures_array_path=cx.DataPath(["Small IN100", "Phase Data", "CrystalStructures"]),
                                            generation_algorithm=1, # Discrete = 1 
                                            mask_array_path=cx.DataPath(["Small IN100", "Scan Data", "Mask"]), 
                                            image_geometry_path=cx.DataPath(["Small IN100 Pole Figure"]), 
                                            image_layout=0, # O = Horizontal Layout 
                                            image_prefix=prefix, 
                                            image_size=512, 
                                            lambert_size=64, 
                                            material_name_array_path=cx.DataPath(["Small IN100", "Phase Data", "MaterialName"]), 
                                            num_colors=32, 
                                            output_path="small_in100_pole_figure", 
                                            save_as_image_geometry=True, 
                                            title="Small IN100 Slice 1", 
                                            use_mask=True, 
                                            write_image_to_disk=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the WritePoleFigureFilter")


#------------------------------------------------------------------------------
# Write the DataStructure to a .dream3d file
#------------------------------------------------------------------------------

output_file_path = cxtest.GetTestTempDirectory() + "/basic_ebsd_example.dream3d"
print(f'{output_file_path}')
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the WriteDREAM3DFilter")

