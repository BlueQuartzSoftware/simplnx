import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxitk.ITKImportImageStack()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    image_data_array_path="ImageData",
    image_geometry_path=cx.DataPath("ImageDataContainer"),
    image_transform_choice=0,
    input_file_list_info=cx.DataPath("Data/Porosity_Image/"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ITKImportImageStack")

# Filter 2
# Set Up Thresholds and Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["ImageDataContainer/Cell Data/ImageData"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = cx.DataType.boolean

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=dt,
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
    print(f"{filter.name()} No errors running the MultiThresholdObjects")

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
    print(f"{filter.name()} No errors running the ScalarSegmentFeaturesFilter")

# Filter 4
# Instantiate Filter
filter = cx.QuickSurfaceMeshFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_data_group_name="Face Data",
    face_feature_attribute_matrix_name="Face Feature Data",
    face_labels_array_name="FaceLabels",
    feature_ids_path=cx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    fix_problem_voxels=True,
    generate_triple_lines=False,
    grid_geometry_data_path=cx.DataPath("ImageDataContainer"),
    node_types_array_name="NodeTypes",
    selected_data_array_paths=[],
    triangle_geometry_name=cx.DataPath("TriangleDataContainer"),
    vertex_data_group_name="Vertex Data"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the QuickSurfaceMeshFilter")


# Filter 5
# Instantiate Filter
filter = cx.LaplacianSmoothingFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    iteration_steps=15,
    lambda_value=0.2,
    mu_factor=0.2,
    quad_point_lambda=0.1,
    surface_mesh_node_type_array_path=cx.DataPath("TriangleDataContainer/Vertex Data/NodeTypes"),
    surface_point_lambda=0.0,
    surface_quad_point_lambda=0.0,
    surface_triple_line_lambda=0.0,
    triangle_geometry_data_path=cx.DataPath("TriangleDataContainer"),
    triple_line_lambda=0.1,
    use_taubin_smoothing=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the LaplacianSmoothingFilter")

# Filter 6
# Instantiate Filter
filter = cx.SplitAttributeArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    delete_original_array=True,
    multicomponent_array=cx.DataPath("TriangleDataContainer/Face Data/FaceLabels"),
    postfix="-",
    select_components_to_extract=False
    # components_to_extract: List[List[float]] = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the SplitAttributeArrayFilter")


# Filter 7
# Set Up Thresholds and Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["TriangleDataContainer/Face Data/FaceLabels-0"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
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
    created_data_path="FaceLabels-0 Mask",
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
    print(f"{filter.name()} No errors running the MultiThresholdObjects")


# Filter 8
# Set Up Thresholds and Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["TriangleDataContainer/Face Data/FaceLabels-1"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
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
    created_data_path="FaceLabels-1 Mask",
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
    print(f"{filter.name()} No errors running the MultiThresholdObjects")

# Filter 9
# Instantiate Filter
filter = cx.ConditionalSetValue()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    conditional_array_path=cx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0 Mask"),
    invert_mask=False,
    replace_value="1",
    selected_array_path=cx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0"),
    use_conditional=True
    # remove_value: str = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ConditionalSetValue")


# Filter 10
# Instantiate Filter
filter = cx.ConditionalSetValue()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    conditional_array_path=cx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1 Mask"),
    invert_mask=False,
    replace_value="1",
    selected_array_path=cx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1"),
    use_conditional=True
    # remove_value: str = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ConditionalSetValue")

# Filter 11
# Instantiate Filter
filter = cx.CombineAttributeArraysFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    move_values=True,
    normalize_data=False,
    stacked_data_array_name="Face Labels",
    selected_data_array_paths=[
        cx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0"),
        cx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1")
    ]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the CombineAttributeArraysFilter")

# Filter 12
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = "Data/Output/Porosity_Analysis.dream3d"
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the WriteDREAM3DFilter")
