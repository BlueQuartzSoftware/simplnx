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
    output_image_geometry_path =nx.DataPath("DataContainer"),
    input_file=nxtest.get_data_directory() / "Small_IN100/Slice_1.ang",
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis_angle=[0.0, 0.0, 1.0, 90.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    output_image_geometry_path=nx.DataPath("beans"),
    remove_original_geometry=False,
    rotate_slice_by_slice=True,
    rotation_axis_angle=[0.0, 1.0, 0.0, 180.0],
    rotation_representation_index=0,
    input_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.MultiThresholdObjectsFilter()
# Set Threshold Conditions
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("DataContainer/Cell Data/Confidence Index")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.LessThan
threshold_1.value = 0.1

# Create a Threshold Set
threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds_object=threshold_set,
    output_data_array_name="Mask",
    created_mask_type=nx.DataType.boolean
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.ConditionalSetValueFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    conditional_array_path=nx.DataPath("DataContainer/Cell Data/Mask"),
    invert_mask=False,
    remove_value="0",
    selected_array_path=nx.DataPath("DataContainer/Cell Data/Confidence Index"),
    use_conditional=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = nx.ComputeArrayHistogramFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #data_group_name=nx.DataPath("DataContainer/Statistics"),
    histogram_bin_count_suffix=" Histogram",
    histogram_bin_range_suffix=" Histogram Ranges",
    max_range=1.0,
    min_range=0.0,
    create_new_data_group=True,
    new_data_group_path=nx.DataPath("DataContainer/Statistics"),
    number_of_bins=25,
    selected_array_paths=[nx.DataPath("DataContainer/Cell Data/Confidence Index")],
    user_defined_range=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 7
# Instantiate Filter
nx_filter = nx.WriteASCIIDataFilter()
output_file_path = nxtest.get_data_directory() / "Output/OrientationAnalysis/Test/CI_Histogram.csv"
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    #file_extension: str = ...,
    delimiter_index=2,
    header_option_index=1,
    #max_val_per_line: int = ...,
    #output_dir: PathLike = ...,
    output_path=output_file_path,
    output_style_index=1,
    input_data_array_paths=[nx.DataPath("DataContainer/Statistics/Confidence Index Histogram")]
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************

print("===> Pipeline Complete")
