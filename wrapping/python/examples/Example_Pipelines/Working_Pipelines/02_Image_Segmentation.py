import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1

generated_file_list_value = cx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/Porosity_Image/"
generated_file_list_value.ordering = cx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2

# Instantiate Filter
filter = cxitk.ITKImportImageStack()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    image_data_array_path="ImageData",
    image_geometry_path=cx.DataPath("ImageDataContainer"),
    image_transform_choice=0,
    input_file_list_info=generated_file_list_value,
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Set Up Thresholds and Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("ImageDataContainer/Cell Data/ImageData")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = cx.DataType.boolean  # This line specifies the DataType for the threshold

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=dt,  # Use the DataType variable here
    use_custom_false_value=False,
    use_custom_true_value=False
    # custom_false_value: float = ...,  # Not currently part of the code
    # custom_true_value: float = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 3
# Instantiate Filter
filter = cx.ScalarSegmentFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    active_array_path="Active",
    cell_feature_group_path="CellFeatureData",
    feature_ids_path="FeatureIds",
    grid_geometry_path=cx.DataPath("ImageDataContainer"),
    input_array_path=cx.DataPath("ImageDataContainer/Cell Data/ImageData"),
    mask_path=cx.DataPath("ImageDataContainer/Cell Data/Mask"),
    randomize_features=True,
    scalar_tolerance=0,
    use_mask=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.CalculateFeatureSizesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    equivalent_diameters_path="EquivalentDiameters",
    feature_attribute_matrix=cx.DataPath("ImageDataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    geometry_path=cx.DataPath("ImageDataContainer"),
    num_elements_path="NumElements",
    save_element_sizes=False,
    volumes_path="Volumes"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cx.CopyFeatureArrayToElementArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_array_suffix="Created Array Suffix",
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    selected_feature_array_path=[cx.DataPath("ImageDataContainer/CellFeatureData/EquivalentDiameters")]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    component_count=1,
    data_format="",
    initialization_value="1",
    numeric_type=cx.NumericType.int32,
    output_data_array=cx.DataPath("ImageDataContainer/Cell Data/Phases")
    # tuple_dimensions: List[List[float]] = ...  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.ConditionalSetValue()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    conditional_array_path=cx.DataPath("ImageDataContainer/Cell Data/Mask"),
    invert_mask=False,
    replace_value="2",
    selected_array_path=cx.DataPath("ImageDataContainer/Cell Data/Phases"),
    use_conditional=True
    # remove_value: str = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.FindFeaturePhasesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_features_attribute_matrix_path=cx.DataPath("ImageDataContainer/CellFeatureData"),
    cell_phases_array_path=cx.DataPath("ImageDataContainer/Cell Data/Phases"),
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    feature_phases_array_name="Phases"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")
# Filter 9

# Instantiate Filter
filter = cx.FindFeatureCentroidsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_path="Centroids",
    feature_attribute_matrix=cx.DataPath("ImageDataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    selected_image_geometry=cx.DataPath("ImageDataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 10
# Instantiate Filter
filter = cx.CreateAttributeMatrixFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    data_object_path=cx.DataPath("Ensemble AttributeMatrix"),
    tuple_dimensions=[[3.0]]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 11
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = "Data/Output/ImagesStack/Images.dream3d"
result = filter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

    print("===> Pipeline Complete")