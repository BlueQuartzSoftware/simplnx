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
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
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
    #created_image_geometry=cx.DataPath("DataContainer"),
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix: List[List[float]] = ...,
    rotation_representation=0,
    selected_image_geometry=cx.DataPath("DataContainer")
)

#Filter 4

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
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

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=cx.DataPath("DataContainer/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    good_voxels_array_path=cx.DataPath("DataContainer/Cell Data/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=True
)

#Filter 6

result = cxitk.ITKImageWriter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/Edax_IPF_Colors/Small_IN100_Slice_1.png"),
    image_array_path=cx.DataPath("DataContainer/Cell Data/IPFColors"),
    image_geom_path=cx.DataPath("DataContainer"),
    index_offset=0,
    plane=0
)