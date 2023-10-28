import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadH5EbsdFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_filter=cx.DataPath("Data/Output/Reconstruction/Small_IN100.h5ebsd"),
)
#FIlter 2

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["DataContainer/CellData/Image Quality"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath(["DataContainer/CellData/Confidence Index"])
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = cx.DataType.boolean

result = cx.MultiThresholdObjects.execute(
                                        data_structure=data_structure,
                                        array_thresholds=threshold_set,
                                        created_data_path="Mask",
                                        created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the MultiThresholdObjects")

#Filter 3

result = cxor.ConvertOrientations.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("DataContainer/CellData/EulerAngles"),
    input_type=0,
    output_orientation_array_name="Quats",
    output_type=2
)
#Filter 4

result = cx.ScalarSegmentFeaturesFilter.execute(
    data_structure=data_structure,
    active_array_path="Active",
    cell_feature_group_path="CellFeatureData",
    feature_ids_path="FeatureIds",
    grid_geometry_path=cx.DataPath("DataContainer"),
    #input_array_path: DataPath = ...,
    mask_path=cx.DataPath("DataContainer/CellData/Mask"),
    randomize_features=True,
    scalar_tolerance=5,
    use_mask=True,
)
#Filter 5

result = cx.FindLargestCrossSectionsFilter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry_path=cx.DataPath("DataContainer"),
    largest_cross_sections_array_path="LargestCrossSections",
    plane=0
)
#Filter 6

output_file_path = "Data/Output/Examples/SmallIN100_LargestCrossSections.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")