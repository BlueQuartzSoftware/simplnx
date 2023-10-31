import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxitk.ITKImportImageStack.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    image_data_array_path="ImageData",
    image_geometry_path=cx.DataPath("ImageDataContainer"),
    image_transform_choice=0,
    input_file_list_info=cx.DataPath("Data/Porosity_Image/"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)

#Filter 2

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["ImageDataContainer/Cell Data/ImageData"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = cx.DataType.boolean

result = cx.MultiThresholdObjects.execute(
    data_structure=data_structure,
    array_thresholds=cx.ArrayThresholdSet,
    created_data_path="Mask",
    created_mask_type=9,
    #custom_false_value: float = ...,
    #custom_true_value: float = ...,
    use_custom_false_value=False,
    use_custom_true_value=False
)

#Filter 3

result = cx.ScalarSegmentFeaturesFilter.execute(
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

#Filter 4

result = cx.CalculateFeatureSizesFilter.execute(
    data_structure=data_structure,
    equivalent_diameters_path=("EquivalentDiameters"),
    feature_attribute_matrix=cx.DataPath("ImageDataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    geometry_path=cx.DataPath("ImageDataContainer"),
    num_elements_path=("NumElements"),
    save_element_sizes=False,
    volumes_path=("Volumes")
)

#Filter 5

result = cx.CopyFeatureArrayToElementArray.execute(
    data_structure=data_structure,
    created_array_suffix="Created Array Suffix",
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    selected_feature_array_path=[cx.DataPath("ImageDataContainer/CellFeatureData/EquivalentDiameters")]
)

#Filter 6

result = cx.CreateDataArray.execute(
    data_structure=data_structure,
    advanced_options=False,
    component_count=1,
    data_format=("Unknown"),
    initialization_value=("1"),
    numeric_type=4,
    output_data_array=cx.DataPath("Phases")
    #tuple_dimensions: List[List[float]] = ...
)

#Filter 7

result = cx.ConditionalSetValue.execute(
    data_structure=data_structure,
    conditional_array_path=cx.DataPath("ImageDataContainer/Cell Data/Mask"),
    invert_mask=False,
    #remove_value: str = ...,
    replace_value=("2"),
    selected_array_path=cx.DataPath("ImageDataContainer/Cell Data/Phases"),
    use_conditional=True
)

#Filter 8

result = cx.FindFeaturePhasesFilter.execute(
    data_structure=data_structure,
    cell_features_attribute_matrix_path=("ImageDataContainer/CellFeatureData"),
    cell_phases_array_path=cx.DataPath("ImageDataContainer/Cell Data/Phases"),
    feature_ids_path=("ImageDataContainer/Cell Data/FeatureIds"),
    feature_phases_array_path=("Phases")
)

#Filter 9

result = cx.FindFeatureCentroidsFilter.execute(
    data_structure=data_structure,
    centroids_array_path=("Centroids"),
    feature_attribute_matrix=cx.DataPath("ImageDataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    selected_image_geometry=cx.DataPath("ImageDataContainer")
)

#Filter 10

result = cx.CreateAttributeMatrixFilter.execute(
    data_structure=data_structure,
    data_object_path=cx.DataPath("Ensemble AttributeMatrix"),
    tuple_dimensions=[3.0]
)

#Filter 11

output_file_path = "Data/Output/ImagesStack/Images.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")