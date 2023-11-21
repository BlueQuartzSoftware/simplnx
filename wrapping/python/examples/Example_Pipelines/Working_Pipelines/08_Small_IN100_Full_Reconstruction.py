import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter

filter_parameter = cxor.ReadH5EbsdFileParameter.ValueType()
filter_parameter.euler_representation=0
filter_parameter.end_slice=117
filter_parameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
filter_parameter.input_file_path="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/Reconstruction/Small_IN100.h5ebsd"
filter_parameter.start_slice=6
filter_parameter.use_recommended_transform=True

filter = cxor.ReadH5EbsdFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_parameter=filter_parameter
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Set Up Thresholds and Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("DataContainer/CellData/Image Quality")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 120

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath("DataContainer/CellData/Confidence Index")
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 0.1

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
dt = cx.DataType.boolean

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=cx.DataType.boolean
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
filter = cxor.AlignSectionsMisorientationFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
    use_mask=True,
    write_alignment_shifts=False
    # alignment_shift_file_name: PathLike = ...,  # Not currently part of the code
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
filter = cx.IdentifySample()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    fill_holes=False,
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    image_geometry=cx.DataPath("DataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the IdentifySample")

# Filter 6
# Instantiate Filter
filter = cx.AlignSectionsFeatureCentroidFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    reference_slice=0,
    selected_cell_data_path=cx.DataPath("DataContainer/CellData"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
    use_reference_slice=True,
    write_alignment_shifts=False
    # alignment_shift_file_name: PathLike = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 7
# Instantiate Filter
filter = cxor.BadDataNeighborOrientationCheckFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    image_geometry_path=cx.DataPath("DataContainer"),
    misorientation_tolerance=5.0,
    number_of_neighbors=4,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cxor.NeighborOrientationCorrelationFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    correlation_array_path=cx.DataPath("DataContainer/CellData/Confidence Index"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    image_geometry_path=cx.DataPath("DataContainer"),
    level=2,
    min_confidence=0.2,
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats")
    # ignored_data_array_paths: List[DataPath] = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 9
# Instantiate Filter
filter = cxor.EBSDSegmentFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    cell_feature_attribute_matrix_name="CellFeatureData",
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_array_name="FeatureIds",
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    grid_geometry_path=cx.DataPath("DataContainer"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    randomize_features=True,
    use_mask=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 10
# Instantiate Filter
filter = cx.FindFeaturePhasesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_features_attribute_matrix_path=cx.DataPath("DataContainer/CellFeatureData"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_phases_array_name="Phases"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 11
# Instantiate Filter
filter = cxor.FindAvgOrientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path="AvgEulerAngles",
    avg_quats_array_path="AvgQuats",
    cell_feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    cell_feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    cell_quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 12
# Instantiate Filter
filter = cx.FindNeighbors()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list="NeighborList2",
    number_of_neighbors="NumNeighbors2",
    shared_surface_area_list="SharedSurfaceAreaList2",
    store_boundary_cells=False,
    store_surface_features=False
    # boundary_cells: str = ...,  # Not currently part of the code
    # surface_features: str = ...  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 13
# Instantiate Filter
filter = cxor.MergeTwinsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    angle_tolerance=2.0,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    axis_tolerance=3.0,
    cell_parent_ids_array_name="ParentIds",
    contiguous_neighbor_list_array_path=cx.DataPath("DataContainer/CellFeatureData/NeighborList2"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_parent_ids_array_name="ParentIds",
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    new_cell_feature_attribute_matrix_name="NewGrain Data",
    use_non_contiguous_neighbors=False
    # non_contiguous_neighbor_list_array_path: DataPath = ...,  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 14
# Instantiate Filter
filter = cx.CalculateFeatureSizesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    equivalent_diameters_path="EquivalentDiameters",
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    num_elements_path="NumElements",
    save_element_sizes=False,
    volumes_path="Volumes"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 15
# Instantiate Filter
filter = cx.RemoveMinimumSizeFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    apply_single_phase=False,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geom_path=cx.DataPath("DataContainer"),
    min_allowed_features_size=16,
    num_cells_path=cx.DataPath("DataContainer/CellFeatureData/NumElements")
    # feature_phases_path: DataPath = ...,  # Not currently part of the code
    # phase_number: int = ...  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 16
# Instantiate Filter
filter = cx.FindNeighbors()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list="NeighborList",
    number_of_neighbors="NumNeighbors",
    shared_surface_area_list="SharedSurfaceAreaList",
    store_boundary_cells=False,
    store_surface_features=False
    # boundary_cells: str = ...,  # Not currently part of the code
    # surface_features: str = ...  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 17
# Instantiate Filter
filter = cx.MinNeighbors()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    apply_to_single_phase=False,
    cell_attribute_matrix=cx.DataPath("DataContainer/CellData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geom=cx.DataPath("DataContainer"),
    min_num_neighbors=2,
    num_neighbors=cx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
    phase_number=0
    # feature_phases: DataPath = ...,  # Not currently part of the code
    # ignored_voxel_arrays: List[DataPath] = ...  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 18
# Instantiate Filter
filter = cx.FillBadDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    min_allowed_defect_size=1000,
    selected_image_geometry=cx.DataPath("DataContainer"),
    store_as_new_phase=False
    # cell_phases_array_path: DataPath = ...,  # Not currently part of the code
    # ignored_data_array_paths: List[DataPath] = ...  # Not currently part of the code
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 19
# Instantiate Filter
filter = cx.ErodeDilateBadDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    num_iterations=2,
    operation=0,
    selected_image_geometry=cx.DataPath("DataContainer"),
    x_dir_on=True,
    y_dir_on=True,
    z_dir_on=True,
    ignored_data_array_paths=[]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 20
# Instantiate Filter
filter = cx.ErodeDilateBadDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    num_iterations=2,
    operation=1,  # Dilate operation
    selected_image_geometry=cx.DataPath("DataContainer"),
    x_dir_on=True,
    y_dir_on=True,
    z_dir_on=True,
    ignored_data_array_paths=[]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 21
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/CellData/EulerAngles"),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    mask_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 22
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/Reconstruction/SmallIN100_Final.dream3d"
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