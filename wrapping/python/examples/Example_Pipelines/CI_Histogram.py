import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    data_container_name=cx.DataPath("DataContainer"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang"),
)

#Filter 2

result = cxor.RotateEulerRefFrameFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)

#Filter 3

result = cx.RotateSampleRefFrameFilter.execute(
    data_structure=data_structure,
    created_image_geometry=cx.DataPath("DataContainer/"),
    remove_original_geometry=False,
    rotate_slice_by_slice=True,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix: List[List[float]] = ...,
    rotation_representation=0,
    selected_image_geometry=cx.DataPath("DataContainer")
)

#Filter 4

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["DataContainer/Cell Data/Confidence Index"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.LessThan
threshold_1.value = 0.1

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the MultiThresholdObjects")

#Filter 5

result = cx.ConditionalSetValue.execute(
    data_structure=data_structure,
    conditional_array_path=cx.DataPath("DataContainer/Cell Data/Mask"),
    invert_mask=False,
    remove_value=("0"),
    #replace_value: str = ...,
    selected_array_path=cx.DataPath("DataContainer/Cell Data/Confidence Index"),
    use_conditional=True
)

#Filter 6

result = cx.CalculateArrayHistogramFilter.execute(
    data_structure=data_structure,
    data_group_name=cx.DataPath("Statistics"),
    histogram_suffix=("Histogram"),
    max_range=("1"),
    min_range=("0"),
    new_data_group=True,
    new_data_group_name=cx.DataPath("DataContainer/"),
    number_of_bins=25,
    selected_array_paths=[cx.DataPath("DataContainer/Cell Data/Confidence Index")],
    user_defined_range=True
)

#Filter 7

result = cx.WriteASCIIDataFilter.execute(
    data_structure=data_structure,
    delimiter=2,
    #file_extension: str = ...,
    includes=1,
    #max_val_per_line: int = ...,
    #output_dir=cx.DataPath(""),
    output_path=cx.DataPath("Data/Output/OrientationAnalysis/Test/CI_Histogram.csv"),
    output_style=1,
    selected_data_array_paths=[cx.DataPath("DataContainer/Statistics/Confidence Index Histogram")]
)