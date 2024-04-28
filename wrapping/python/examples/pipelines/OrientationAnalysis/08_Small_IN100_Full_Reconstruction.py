import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter

filter_parameter = cxor.ReadH5EbsdFileParameter.ValueType()
filter_parameter.euler_representation=0
filter_parameter.end_slice=117
filter_parameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
filter_parameter.input_file_path=str(nxtest.get_data_directory() / "Output/Reconstruction/Small_IN100.h5ebsd")
filter_parameter.start_slice=6
filter_parameter.use_recommended_transform=True

nx_filter = cxor.ReadH5EbsdFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    output_image_geometry_path =nx.DataPath("DataContainer"),
    read_h5_ebsd_object=filter_parameter
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Set Up Thresholds and Instantiate Filter
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("DataContainer/CellData/Image Quality")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 120

threshold_2 = nx.ArrayThreshold()
threshold_2.array_path = nx.DataPath("DataContainer/CellData/Confidence Index")
threshold_2.comparison = nx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 0.1

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = nx.DataType.boolean

# Instantiate Filter
nx_filter = nx.MultiThresholdObjects()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds_object=threshold_set,
    output_data_array_name="Mask",
    created_mask_type=nx.DataType.boolean
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = cxor.ConvertOrientations()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=nx.DataPath("DataContainer/CellData/EulerAngles"),
    input_representation_index=0,
    output_orientation_array_name="Quats",
    output_representation_index=2

)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = cxor.AlignSectionsMisorientationFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    misorientation_tolerance=5.0,
    quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    use_mask=True,
    write_alignment_shifts=False
    # alignment_shift_file_name: PathLike = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.IdentifySampleFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    fill_holes=False,
    mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    input_image_geometry_path =nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = nx.AlignSectionsFeatureCentroidFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    reference_slice=0,
    selected_cell_data_path=nx.DataPath("DataContainer/CellData"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    use_reference_slice=True,
    write_alignment_shifts=False
    # alignment_shift_file_name: PathLike = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 7
# Instantiate Filter
nx_filter = cxor.BadDataNeighborOrientationCheckFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    misorientation_tolerance=5.0,
    number_of_neighbors=4,
    quats_array_path=nx.DataPath("DataContainer/CellData/Quats")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 8
# Instantiate Filter
nx_filter = cxor.NeighborOrientationCorrelationFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    correlation_array_path=nx.DataPath("DataContainer/CellData/Confidence Index"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    level=2,
    min_confidence=0.2,
    misorientation_tolerance=5.0,
    quats_array_path=nx.DataPath("DataContainer/CellData/Quats")
    # ignored_data_array_paths: List[DataPath] = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 9
# Instantiate Filter
nx_filter = cxor.EBSDSegmentFeaturesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    cell_feature_attribute_matrix_name="CellFeatureData",
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_array_name="FeatureIds",
    cell_mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    input_image_geometry_path =nx.DataPath("DataContainer"),
    misorientation_tolerance=5.0,
    cell_quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    randomize_features=True,
    use_mask=True
)
nxtest.check_filter_result(nx_filter, result)


# Filter 10
# Instantiate Filter
nx_filter = nx.FindFeaturePhasesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_features_attribute_matrix_path=nx.DataPath("DataContainer/CellFeatureData"),
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_phases_array_name="Phases"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 11
# Instantiate Filter
nx_filter = cxor.FindAvgOrientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_name="AvgEulerAngles",
    avg_quats_array_name="AvgQuats",
    cell_feature_attribute_matrix_path=nx.DataPath("DataContainer/CellFeatureData"),
    cell_feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    cell_quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 12
# Instantiate Filter
nx_filter = nx.FindNeighborsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_feature_array_path=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    input_image_geometry_path =nx.DataPath("DataContainer"),
    neighbor_list_name="NeighborList2",
    number_of_neighbors_name="NumNeighbors2",
    shared_surface_area_list_name="SharedSurfaceAreaList2",
    store_boundary_cells=False,
    store_surface_features=False
    # boundary_cells: str = ...,  # Not currently part of the code
    # surface_features: str = ...  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 13
# Instantiate Filter
nx_filter = cxor.MergeTwinsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    angle_tolerance=2.0,
    avg_quats_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    axis_tolerance=3.0,
    cell_parent_ids_array_name="ParentIds",
    contiguous_neighbor_list_array_path=nx.DataPath("DataContainer/CellFeatureData/NeighborList2"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_parent_ids_array_name="ParentIds",
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    created_feature_attribute_matrix_name="NewGrain Data",
    use_non_contiguous_neighbors=False
    # non_contiguous_neighbor_list_array_path: DataPath = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 14
# Instantiate Filter
nx_filter = nx.CalculateFeatureSizesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    equivalent_diameters_name="EquivalentDiameters",
    feature_attribute_matrix_path=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    num_elements_name="NumElements",
    save_element_sizes=False,
    volumes_name="Volumes"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 15
# Instantiate Filter
nx_filter = nx.RemoveMinimumSizeFeaturesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    apply_single_phase=False,
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    min_allowed_features_size=16,
    num_cells_path=nx.DataPath("DataContainer/CellFeatureData/NumElements")
    # feature_phases_path: DataPath = ...,  # Not currently part of the code
    # phase_number: int = ...  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 16
# Instantiate Filter
nx_filter = nx.FindNeighborsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_feature_array_path=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    input_image_geometry_path =nx.DataPath("DataContainer"),
    neighbor_list_name="NeighborList",
    number_of_neighbors_name="NumNeighbors",
    shared_surface_area_list_name="SharedSurfaceAreaList",
    store_boundary_cells=False,
    store_surface_features=False
    # boundary_cells: str = ...,  # Not currently part of the code
    # surface_features: str = ...  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 17
# Instantiate Filter
nx_filter = nx.MinNeighborsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    apply_to_single_phase=False,
    cell_attribute_matrix_path=nx.DataPath("DataContainer/CellData"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    input_image_geometry_path=nx.DataPath("DataContainer"),
    min_num_neighbors=2,
    num_neighbors_path=nx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
    phase_number=0
    # feature_phases: DataPath = ...,  # Not currently part of the code
    # ignored_voxel_arrays: List[DataPath] = ...  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 18
# Instantiate Filter
nx_filter = nx.FillBadDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    min_allowed_defect_size=1000,
    input_image_geometry_path=nx.DataPath("DataContainer"),
    store_as_new_phase=False
    # cell_phases_array_path: DataPath = ...,  # Not currently part of the code
    # ignored_data_array_paths: List[DataPath] = ...  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)


# Filter 19
# Instantiate Filter
nx_filter = nx.ErodeDilateBadDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    num_iterations=2,
    operation_index=0,
    input_image_geometry_path=nx.DataPath("DataContainer"),
    x_dir_on=True,
    y_dir_on=True,
    z_dir_on=True,
    ignored_data_array_paths=[]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 20
# Instantiate Filter
nx_filter = nx.ErodeDilateBadDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    num_iterations=2,
    operation_index=1,  # Dilate operation
    input_image_geometry_path=nx.DataPath("DataContainer"),
    x_dir_on=True,
    y_dir_on=True,
    z_dir_on=True,
    ignored_data_array_paths=[]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 21
# Instantiate Filter
nx_filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("DataContainer/CellData/EulerAngles"),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=True
)
nxtest.check_filter_result(nx_filter, result)


# Filter 22
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.get_data_directory() / "Output/Reconstruction/SmallIN100_Final.dream3d"
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(filter_parameter.input_file_path)
# *****************************************************************************


print("===> Pipeline Complete")
