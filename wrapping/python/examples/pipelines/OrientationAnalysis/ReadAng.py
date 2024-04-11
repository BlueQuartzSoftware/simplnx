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
    cell_ensemble_attribute_matrix_name=("Cell Ensemble Data"),
    created_image_geometry_path =nx.DataPath("DataContainer"),
    input_file=nxtest.get_data_directory() / "Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #created_image_geometry: DataPath = ...,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix: Any = ...,
    rotation_representation=0,
    selected_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Define ArrayThreshold object
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("DataContainer/Cell Data/Confidence Index")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.LessThan
threshold_1.value = 0.1

# Define ArrayThresholdSet object
threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Instantiate Filter
nx_filter = nx.MultiThresholdObjects()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        array_thresholds_object=threshold_set,
                        created_data_array_name="Mask",
                        created_mask_type=nx.DataType.boolean)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=nx.DataPath("DataContainer/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/Cell Ensemble Data/CrystalStructures"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
)
nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")
