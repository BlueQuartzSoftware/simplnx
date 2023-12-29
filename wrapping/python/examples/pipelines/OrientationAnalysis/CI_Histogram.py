import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    data_container_name=nx.DataPath("DataContainer"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang",
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    euler_angles_array_path=nx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_image_geometry=nx.DataPath("beans"),
    remove_original_geometry=False,
    rotate_slice_by_slice=True,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    rotation_representation=0,
    selected_image_geometry=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(filter, result)

# Filter 4
# Instantiate Filter
filter = nx.MultiThresholdObjects()
# Set Threshold Conditions
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("DataContainer/Cell Data/Confidence Index")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.LessThan
threshold_1.value = 0.1

# Create a Threshold Set
threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=nx.DataType.boolean
)
nxtest.check_filter_result(filter, result)

# Filter 5
# Instantiate Filter
filter = nx.ConditionalSetValue()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    conditional_array_path=nx.DataPath("DataContainer/Cell Data/Mask"),
    invert_mask=False,
    remove_value="0",
    selected_array_path=nx.DataPath("DataContainer/Cell Data/Confidence Index"),
    use_conditional=True
)
nxtest.check_filter_result(filter, result)

# Filter 6
# Instantiate Filter
filter = nx.CalculateArrayHistogramFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #data_group_name=nx.DataPath("DataContainer/Statistics"),
    histogram_suffix=" Histogram",
    max_range=1.0,
    min_range=0.0,
    new_data_group=True,
    new_data_group_name=nx.DataPath("DataContainer/Statistics"),
    number_of_bins=25,
    selected_array_paths=[nx.DataPath("DataContainer/Cell Data/Confidence Index")],
    user_defined_range=True
)
nxtest.check_filter_result(filter, result)

# Filter 7
# Instantiate Filter
filter = nx.WriteASCIIDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #file_extension: str = ...,
    delimiter=2,
    includes=1,
    #max_val_per_line: int = ...,
    #output_dir: PathLike = ...,
    output_path=nxtest.GetDataDirectory() + "/Output/OrientationAnalysis/Test/CI_Histogram.csv",
    output_style=1,
    selected_data_array_paths=[nx.DataPath("DataContainer/Statistics/Confidence Index Histogram")]
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")