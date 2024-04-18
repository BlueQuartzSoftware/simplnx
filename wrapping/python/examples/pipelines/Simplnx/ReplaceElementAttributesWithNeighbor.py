import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    output_image_geometry_path =nx.DataPath("DataContainer"),
    input_file=nxtest.get_data_directory() / "Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate ArrayThreshold objects
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("DataContainer/Cell Data/Confidence Index")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = nx.ArrayThreshold()
threshold_2.array_path = nx.DataPath("DataContainer/Cell Data/Image Quality")
threshold_2.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

# Define ArrayThresholdSet object
threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = nx.DataType.boolean

# Instantiate Filter
nx_filter = nx.MultiThresholdObjects()
# Execute filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds_object=threshold_set,
    output_data_array_name="Mask",
    created_mask_type=nx.DataType.boolean
)
nxtest.check_filter_result(nx_filter, result)


# Filter 3
# Instantiate Filter
nx_filter = nx.RotateSampleRefFrameFilter()
# Execute Filter
result = nx_filter.execute(
    data_structure=data_structure,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis_angle=[0.0, 1.0, 0.0, 180],
    rotation_representation_index=0,
    input_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter
result = nx_filter.execute(
    data_structure=data_structure,
    euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis_angle=[0.0, 0.0, 1.0, 90.0]
)
nxtest.check_filter_result(nx_filter, result)


# Filter 5
# Instantiate Filter
nx_filter = nx.SetImageGeomOriginScalingFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    change_origin=True,
    change_spacing=True,
    input_image_geometry_path=nx.DataPath("DataContainer"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = nx.CropImageGeometry()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix: DataPath = ...,
    #created_image_geometry: DataPath = ...,
    #feature_ids: DataPath = ...,
    max_voxel=[160, 175, 0],
    min_voxel=[25, 25, 0],
    remove_original_geometry=True,
    renumber_features=False,
    input_image_geometry_path=nx.DataPath("DataContainer")
   # update_origin=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 7
# Instantiate Filter
nx_filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=nx.DataPath("DataContainer/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=nx.DataPath("DataContainer/Cell Data/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 8
# Instantiate Filter
nx_filter = nx.ReplaceElementAttributesWithNeighborValuesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    comparison_data_path=nx.DataPath("DataContainer/Cell Data/Confidence Index"),
    loop=True,
    min_confidence=0.1,
    comparison_index=0,
    input_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 9
output_file_path = nxtest.get_data_directory() / "Output/Examples/ReplaceElementAttributesWithNeighbor.dream3d"
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************


print("===> Pipeline Complete")
