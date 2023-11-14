import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadH5EbsdFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_filter="Data/Output/Reconstruction/Small_IN100.h5ebsd"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
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

# Execute Filter with Parameters
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                            array_thresholds=threshold_set,
                                            created_data_path = "Mask",
                                            created_mask_type = cx.DataType.boolean,
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
filter = cxor.ConvertOrientations()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("DataContainer/CellData/EulerAngles"),
    input_type=0,
    output_orientation_array_name="Quats",
    output_type=2
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
filter = cx.ScalarSegmentFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    active_array_path="Active",
    cell_feature_group_path="CellFeatureData",
    feature_ids_path="FeatureIds",
    grid_geometry_path=cx.DataPath("DataContainer"),
    mask_path=cx.DataPath("DataContainer/CellData/Mask"),
    randomize_features=True,
    scalar_tolerance=5,
    use_mask=True
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
filter = cx.FindLargestCrossSectionsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry_path=cx.DataPath("DataContainer"),
    largest_cross_sections_array_path="LargestCrossSections",
    plane=0
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 6
# Define output file path
output_file_path = "Data/Output/Examples/SmallIN100_LargestCrossSections.dream3d"
# Execute ExportDREAM3DFilter with Parameters
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True
)
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")