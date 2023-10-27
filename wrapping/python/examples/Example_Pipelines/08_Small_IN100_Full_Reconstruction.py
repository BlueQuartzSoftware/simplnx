import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadH5EbsdFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("CellData"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    data_container_name=cx.DataPath("DataContainer"),
    read_h5_ebsd_filter=cx.DataPath("Data/Output/Reconstruction/Small_IN100.h5ebsd"),
)
#Filter 2

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
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)

#Filter 3

result = cxor.ConvertOrientations.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("DataContainer/CellData/EulerAngles"),
    input_type=0,
    output_orientation_array_name=("Quats"),
)
#Filter 4

result = cxor.AlignSectionsMisorientationFilter.execute(
    data_structure=data_structure,
    #alignment_shift_file_name: PathLike = ...,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    good_voxels_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
    use_good_voxels=True,
    write_alignment_shifts=False,
)
#Filter 5

result = cx.IdentifySample.execute(
    data_structure=data_structure,
    fill_holes=False,
    good_voxels=cx.DataPath("DataContainer/CellData/Mask"),
    image_geometry=cx.DataPath("DataContainer"),
)
#Filter 6

result = cx.AlignSectionsFeatureCentroidFilter.execute(
    data_structure=data_structure,
    #alignment_shift_file_name: PathLike = ...,
    good_voxels_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    reference_slice=0,
    selected_cell_data_path=cx.DataPath("DataContainer/CellData"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
    use_reference_slice=True,
    write_alignment_shifts=False,
)
#Filter 7

result = cxor.BadDataNeighborOrientationCheckFilter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    good_voxels_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    image_geometry_path=cx.DataPath("DataContainer"),
    misorientation_tolerance=5.0,
    number_of_neighbors=4,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
)
#Filter 8

result = cxor.NeighborOrientationCorrelationFilter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    confidence_index_array_path=cx.DataPath("DataContainer/CellData/Confidence Index"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    #ignored_data_array_paths: List[DataPath] = ...,
    image_geometry_path=cx.DataPath("DataContainer"),
    level=2,
    min_confidence=0.2,
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
)
#Filter 9

result = cxor.EBSDSegmentFeaturesFilter.execute(
    data_structure=data_structure,
    active_array_name=("Active"),
    cell_feature_attribute_matrix_name=("CellFeatureData"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_array_name=("FeatureIds"),
    good_voxels_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    grid_geometry_path=cx.DataPath("DataContainer"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    randomize_features=True,
    use_good_voxels=True,
)
#Filter 10

result = cx.FindFeaturePhasesFilter.execute(
    data_structure=data_structure,
    cell_features_attribute_matrix_path=cx.DataPath("DataContainer/CellFeatureData"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_phases_array_path=("Phases"),
)
#Filter 11

result = cxor.FindAvgOrientationsFilter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path=("AvgEulerAngles"),
    avg_quats_array_path=("AvgQuats"),
    cell_feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    cell_feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    cell_quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
)
#Filter 12

result = cx.FindNeighbors.execute(
    data_structure=data_structure,
    #boundary_cells: str = ...,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list=("NeighborList2"),
    number_of_neighbors=("NumNeighbors2"),
    shared_surface_area_list=("SharedSurfaceAreaList2"),
    store_boundary_cells=False,
    store_surface_features=False,
    #surface_features: str = ...
)
#Filter 13

result = cxor.MergeTwinsFilter.execute(
    data_structure=data_structure,
    active_array_name=("Active"),
    angle_tolerance=2.0,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    axis_tolerance=3.0,
    cell_parent_ids_array_name=("ParentIds"),
    contiguous_neighbor_list_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_parent_ids_array_name=("ParentIds"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    new_cell_feature_attribute_matrix_name=("NewGrain Data"),
    #non_contiguous_neighbor_list_array_path: DataPath = ...,
    use_non_contiguous_neighbors=False,
)
#Filter 14

result = cx.CalculateFeatureSizesFilter.execute(
    data_structure=data_structure,
    equivalent_diameters_path=("EquivalentDiameters"),
    feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    num_elements_path=("NumElements"),
    save_element_sizes=False,
    volumes_path=("Volumes"),
)
#Filter 15

result = cx.RemoveMinimumSizeFeaturesFilter.execute(
    data_structure=data_structure,
    apply_single_phase=False,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    #feature_phases_path: DataPath = ...,
    image_geom_path=cx.DataPath("DataContainer"),
    min_allowed_features_size=16,
    num_cells_path=cx.DataPath("DataContainer/CellFeatureData/NumElements"),
    #phase_number: int = ...
)
#Filter 16

result = cx.FindNeighbors.execute(
    data_structure=data_structure,
    #boundary_cells: str = ...,
    cell_feature_arrays=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry=cx.DataPath("DataContainer"),
    neighbor_list=("NeighborList"),
    number_of_neighbors=("NumNeighbors"),
    shared_surface_area_list=("SharedSurfaceAreaList"),
    store_boundary_cells=False,
    store_surface_features=False,
    #surface_features: str = ...
)
#Filter 17

result = cx.MinNeighbors.execute(
    data_structure=data_structure,
    apply_to_single_phase=True,
    cell_attribute_matrix=cx.DataPath("DataContainer/CellData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    #feature_phases: DataPath = ...,
    #ignored_voxel_arrays: List[DataPath] = ...,
    image_geom=cx.DataPath("DataContainer"),
    min_num_neighbors=2,
    num_neighbors=cx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
    phase_number=0,
)
#Filter 18

result = cx.FillBadDataFilter.execute(
    data_structure=data_structure,
    #cell_phases_array_path: DataPath = ...,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    #ignored_data_array_paths: List[DataPath] = ...,
    min_allowed_defect_size=1000,
    selected_image_geometry=cx.DataPath("DataContainer"),
    store_as_new_phase=False,
)
#Filter 19

result = cx.ErodeDilateBadDataFilter.execute(
    data_structure=data_structure,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    #ignored_data_array_paths: List[DataPath] = ...,
    num_iterations=2,
    operation=0,
    selected_image_geometry=cx.DataPath("DataContainer"),
    x_dir_on=True,
    y_dir_on=True,
    z_dir_on=True,
)
#Filter 20

result = cx.ErodeDilateBadDataFilter.execute(
    data_structure=data_structure,
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    #ignored_data_array_paths: List[DataPath] = ...,
    num_iterations=2,
    operation=1,
    selected_image_geometry=cx.DataPath("DataContainer"),
    x_dir_on=True,
    y_dir_on=True,
    z_dir_on=True,
)
#Filter 21

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    good_voxels_array_path=cx.DataPath("DataContainer/CellData/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=True,
)
#Filter 22

output_file_path = "Data/Output/Reconstruction/SmallIN100_Final.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")
