import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

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
    input_file=cxtest.GetDataDirectory() + "Data/Small_IN100/Slice_1.ang"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate ArrayThreshold objects
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("DataContainer/Cell Data/Confidence Index")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath("DataContainer/Cell Data/Image Quality")
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

# Define ArrayThresholdSet object
threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = cx.DataType.boolean

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute filter with Parameters
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


# Filter 3
# Instantiate Filter
filter = cx.RotateSampleRefFrameFilter()
# Execute Filter
result = filter.execute(
    data_structure=data_structure,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180],
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
filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter
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


# Filter 5
# Instantiate Filter
filter = cx.SetImageGeomOriginScalingFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    change_origin=True,
    change_resolution=True,
    image_geom=cx.DataPath("DataContainer"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
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
filter = cx.CropImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix: DataPath = ...,
    #created_image_geometry: DataPath = ...,
    #feature_ids: DataPath = ...,
    max_voxel=[160, 175, 0],
    min_voxel=[25, 25, 0],
    remove_original_geometry=True,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("DataContainer"),
    update_origin=False
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
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=cx.DataPath("DataContainer/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=cx.DataPath("DataContainer/Cell Data/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.ReplaceElementAttributesWithNeighborValuesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    comparison_data_path=cx.DataPath("DataContainer/Cell Data/Confidence Index"),
    loop=True,
    min_confidence=0.1,
    selected_comparison=0,
    selected_image_geometry=cx.DataPath("DataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 9
output_file_path = "Data/Output/Examples/ReplaceElementAttributesWithNeighbor.dream3d"
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")