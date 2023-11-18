import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    data_container_name=cx.DataPath("DataContainer"),
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/Small_IN100/Slice_1.ang",
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_image_geometry=cx.DataPath("beans"),
    remove_original_geometry=False,
    rotate_slice_by_slice=True,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    rotation_representation=0,
    selected_image_geometry=cx.DataPath("DataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Set Threshold Conditions
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("DataContainer/Cell Data/Confidence Index")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.LessThan
threshold_1.value = 0.1

# Create a Threshold Set
threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=cx.DataType.boolean
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cx.ConditionalSetValue()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    conditional_array_path=cx.DataPath("DataContainer/Cell Data/Mask"),
    invert_mask=False,
    remove_value="0",
    selected_array_path=cx.DataPath("DataContainer/Cell Data/Confidence Index"),
    use_conditional=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cx.CalculateArrayHistogramFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #data_group_name=cx.DataPath("DataContainer/Statistics"),
    histogram_suffix=" Histogram",
    max_range=1.0,
    min_range=0.0,
    new_data_group=True,
    new_data_group_name=cx.DataPath("DataContainer/Statistics"),
    number_of_bins=25,
    selected_array_paths=[cx.DataPath("DataContainer/Cell Data/Confidence Index")],
    user_defined_range=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.WriteASCIIDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #file_extension: str = ...,
    delimiter=2,
    includes=1,
    #max_val_per_line: int = ...,
    #output_dir: PathLike = ...,
    output_path="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/Output/OrientationAnalysis/Test/CI_Histogram.csv",
    output_style=1,
    selected_data_array_paths=[cx.DataPath("DataContainer/Statistics/Confidence Index Histogram")]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")