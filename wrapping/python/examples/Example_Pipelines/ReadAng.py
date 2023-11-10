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
    cell_ensemble_attribute_matrix_name=("Cell Ensemble Data"),
    data_container_name=cx.DataPath("DataContainer"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    rotation_representation=("Axis Angle"),
    selected_image_geometry=cx.DataPath("DataContainer")
)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
elif len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Define ArrayThreshold object
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["DataContainer/Cell Data/Confidence Index"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.LessThan
threshold_1.value = 0.1

# Define ArrayThresholdSet object
threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        array_thresholds=threshold_set,
                        created_data_path="Mask",
                        created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=cx.DataPath("DataContainer/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/Cell Ensemble Data/CrystalStructures"),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=False
)
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    print(f'{filter.name()} Warnings: {result.warnings}')
else:
    print(f"{filter.name()} No errors running the filter")
