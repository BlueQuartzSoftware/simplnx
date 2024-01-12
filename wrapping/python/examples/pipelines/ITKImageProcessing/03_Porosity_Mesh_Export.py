import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = cxitk.ITKImportImageStack()

generated_file_list_value = nx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = nxtest.GetDataDirectory() + "/Porosity_Image/"
generated_file_list_value.ordering = nx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2


# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_data_name="Cell Data",
    image_data_array_path="ImageData",
    image_geometry_path=nx.DataPath("ImageDataContainer"),
    image_transform_choice=0,
    input_file_list_info=generated_file_list_value,
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Set Up Thresholds and Instantiate Filter
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("ImageDataContainer/Cell Data/ImageData")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = nx.DataType.boolean

# Instantiate Filter
nx_filter = nx.MultiThresholdObjects()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=dt,
    use_custom_false_value=False,
    use_custom_true_value=False
    # custom_false_value: float = ...,  # Not currently part of the code
    # custom_true_value: float = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.ScalarSegmentFeaturesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    active_array_path="Active",
    cell_feature_group_path="CellFeatureData",
    feature_ids_path="FeatureIds",
    grid_geometry_path=nx.DataPath("ImageDataContainer"),
    input_array_path=nx.DataPath("ImageDataContainer/Cell Data/ImageData"),
    mask_path=nx.DataPath("ImageDataContainer/Cell Data/Mask"),
    randomize_features=True,
    scalar_tolerance=0,
    use_mask=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.QuickSurfaceMeshFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    face_data_group_name="Face Data",
    face_feature_attribute_matrix_name="Face Feature Data",
    face_labels_array_name="FaceLabels",
    feature_ids_path=nx.DataPath("ImageDataContainer/Cell Data/FeatureIds"),
    fix_problem_voxels=True,
    generate_triple_lines=False,
    grid_geometry_data_path=nx.DataPath("ImageDataContainer"),
    node_types_array_name="NodeTypes",
    selected_data_array_paths=[],
    triangle_geometry_name=nx.DataPath("TriangleDataContainer"),
    vertex_data_group_name="Vertex Data"
)
nxtest.check_filter_result(nx_filter, result)


# Filter 5
# Instantiate Filter
nx_filter = nx.LaplacianSmoothingFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    iteration_steps=15,
    lambda_value=0.2,
    mu_factor=0.2,
    quad_point_lambda=0.1,
    surface_mesh_node_type_array_path=nx.DataPath("TriangleDataContainer/Vertex Data/NodeTypes"),
    surface_point_lambda=0.0,
    surface_quad_point_lambda=0.0,
    surface_triple_line_lambda=0.0,
    triangle_geometry_data_path=nx.DataPath("TriangleDataContainer"),
    triple_line_lambda=0.1,
    use_taubin_smoothing=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = nx.SplitAttributeArrayFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    delete_original_array=True,
    multicomponent_array=nx.DataPath("TriangleDataContainer/Face Data/FaceLabels"),
    postfix="-",
    select_components_to_extract=False
    # components_to_extract: List[List[float]] = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 7
# Set Up Thresholds and Instantiate Filter
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.0

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = nx.DataType.boolean  # This line specifies the DataType for the threshold

# Instantiate Filter
nx_filter = nx.MultiThresholdObjects()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="FaceLabels-0 Mask",
    created_mask_type=dt,  # Use the DataType variable here
    use_custom_false_value=False,
    use_custom_true_value=False
    # custom_false_value: float = ...,  # Not currently part of the code
    # custom_true_value: float = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 8
# Set Up Thresholds and Instantiate Filter
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.0

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = nx.DataType.boolean  # This line specifies the DataType for the threshold

# Instantiate Filter
nx_filter = nx.MultiThresholdObjects()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="FaceLabels-1 Mask",
    created_mask_type=dt,  # Use the DataType variable here
    use_custom_false_value=False,
    use_custom_true_value=False
    # custom_false_value: float = ...,  # Not currently part of the code
    # custom_true_value: float = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 9
# Instantiate Filter
nx_filter = nx.ConditionalSetValue()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    conditional_array_path=nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0 Mask"),
    invert_mask=False,
    replace_value="1",
    selected_array_path=nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0"),
    use_conditional=True
    # remove_value: str = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 10
# Instantiate Filter
nx_filter = nx.ConditionalSetValue()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    conditional_array_path=nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1 Mask"),
    invert_mask=False,
    replace_value="1",
    selected_array_path=nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1"),
    use_conditional=True
    # remove_value: str = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 11
# Instantiate Filter
nx_filter = nx.CombineAttributeArraysFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    move_values=True,
    normalize_data=False,
    stacked_data_array_name="Face Labels",
    selected_data_array_paths=[
        nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-0"),
        nx.DataPath("TriangleDataContainer/Face Data/FaceLabels-1")
    ]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 12
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = nxtest.GetDataDirectory() + "/Output/Porosity_Analysis.dream3d"
result = nx_filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************


print("===> Pipeline Complete")
