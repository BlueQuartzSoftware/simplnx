import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("DataContainer"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang"),
)
#Filter 2

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["DataContainer/Cell Data/Confidence Index"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath(["DataContainer/Cell Data/Image Quality"])
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)

result = cx.MultiThresholdObjects.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=9,
    #custom_false_value: float = ...,
    #custom_true_value: float = ...,
    use_custom_false_value=False,
    use_custom_true_value=False,
)

#Filter 3

result = cx.RotateSampleRefFrameFilter.execute(
    data_structure=data_structure,
    #created_image_geometry: DataPath = ...,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180],
    #rotation_matrix: List[List[float]] = ...,
    rotation_representation=0
    selected_image_geometry=cx.DataPath("DataContainer"),
)
#Filter 4

result = cxor.RotateEulerRefFrameFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0],
)
#Filter 5

result = cx.SetImageGeomOriginScalingFilter.execute(
    data_structure=data_structure,
    change_origin=True,
    change_resolution=True,
    image_geom=cx.DataPath("DataContainer"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0],
)
#Filter 6

result = cx.CropImageGeometry.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix: DataPath = ...,
    #created_image_geometry: DataPath = ...,
    #feature_ids: DataPath = ...,
    max_voxel=[160, 175, 0],
    min_voxel=[25, 25, 0],
    remove_original_geometry=True,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("DataContainer"),
    update_origin=False,
)