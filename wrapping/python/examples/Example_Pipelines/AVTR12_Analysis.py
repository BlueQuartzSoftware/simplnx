import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadCtfDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("CellEnsembleData"),
    data_container_name=cx.DataPath("fw-ar-IF1-avtr12-corr/"),
    degrees_to_radians=True,
    edax_hexagonal_alignment=True,
    input_file=cx.DataPath("Data/T12-MAI-2010/fw-ar-IF1-avtr12-corr.ctf")
)

#Filter 2

result = cx.RotateSampleRefFrameFilter.execute(
    data_structure=data_structure,
    #created_image_geometry=cx.DataPath(),
    remove_original_geometry=False,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix: List[List[float]] = ...,
    rotation_representation=0,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr/")
)

#Filter 3

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["fw-ar-IF1-avtr12-corr/Cell Data/Error"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="ThresholdArray",
                                    created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the MultiThresholdObjects")

#Filter 4

result = cxor.ConvertOrientations.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    input_type=0,
    output_orientation_array_name=("Quats"),
    output_type=2
)

#Filter 5

result = cx.ReplaceElementAttributesWithNeighborValuesFilter.execute(
    data_structure=data_structure,
    confidence_index_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Error"),
    loop=False,
    min_confidence=0,
    selected_comparison=0,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Error")
)

#Filter 6

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPF_001"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    #good_voxels_array_path=cx.DataPath(""),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=False
)

#Filter 7

result = cxitk.ITKImageWriter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_001.png"),
    image_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_001"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane=0
)

#Filter 8

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPF_010"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    #good_voxels_array_path=cx.DataPath(""),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=False
)

#Filter 9

result = cxitk.ITKImageWriter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_010.png"),
    image_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_010"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane=0
)

#Filter 10

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPF_100"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    #good_voxels_array_path=cx.DataPath(""),
    reference_dir=[1.0, 0.0, 0.0],
    use_good_voxels=False
)

#Filter 11

result = cxitk.ITKImageWriter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_100.png"),
    image_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_100"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane=0
)

#Filter 12

result = cxor.CAxisSegmentFeaturesFilter.execute(
    data_structure=data_structure,
    active_array_name=("Active"),
    cell_feature_attribute_matrix_name=("CellFeatureData"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_ids_array_name=("FeatureIds"),
    good_voxels_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/ThresholdArray"),
    image_geometry_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    randomize_feature_ids=True,
    use_good_voxels=True
)

#Filter 13

result = cx.FillBadDataFilter.execute(
    data_structure=data_structure,
    #cell_phases_array_path=cx.DataPath(""),
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    #ignored_data_array_paths: List[DataPath] = ...,
    min_allowed_defect_size=10,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    store_as_new_phase=False
)

#Filter 14

result = cx.CalculateFeatureSizesFilter.execute(
    data_structure=data_structure,
    equivalent_diameters_path=("EquivalentDiameters"),
    feature_attribute_matrix=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    geometry_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    num_elements_path=("NumElements"),
    save_element_sizes=False,
    volumes_path=("Volumes")
)

#Filter 15

result = cx.RemoveMinimumSizeFeaturesFilter.execute(
    data_structure=data_structure,
    apply_single_phase=False,
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    #feature_phases_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Phases"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    min_allowed_features_size=5,
    num_cells_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData/NumElements")
    #phase_number: int = ...
)

#Filter 16

result = cxor.FindAvgOrientationsFilter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path=("AvgEulerAngles"),
    avg_quats_array_path=("AvgQuats"),
    cell_feature_attribute_matrix=cx.DataPath("fw-ar-IF1-aptr-corr/CellFeatureData"),
    cell_feature_ids_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/FeatureIds"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Phases"),
    cell_quats_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Quats"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/CellEnsembleData/CrystalStructures")
)

#Filter 17

result = cxor.FindKernelAvgMisorientationsFilter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/CellEnsembleData/CrystalStructures"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/FeatureIds"),
    kernel_average_misorientations_array_name=("KernelAverageMisorientations"),
    kernel_size=[1, 1, 1],
    quats_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Quats"),
    selected_image_geometry_path=cx.DataPath("fw-ar-IF1-aptr-corr")
)

#Filter 18

result = cx.FindFeatureCentroidsFilter.execute(
    data_structure=data_structure,
    centroids_array_path=("Centroids"),
    feature_attribute_matrix=cx.DataPath("fw-ar-IF1-aptr-corr/CellFeatureData"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/FeatureIds"),
    selected_image_geometry=cx.DataPath("fw-ar-IF1-aptr-corr")
)

#Filter 19

result = cx.FindEuclideanDistMapFilter.execute(
    data_structure=data_structure,
    calc_manhattan_dist=True,
    do_boundaries=True,
    do_quad_points=False,
    do_triple_lines=False,
    feature_ids_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/FeatureIds"),
    g_bdistances_array_name=("GBManhattanDistances"),
    #nearest_neighbors_array_name: str = ...,
    #q_pdistances_array_name: str = ...,
    save_nearest_neighbors=False,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-aptr-corr")
    #t_jdistances_array_name: str = ...
)

#Filter 20

result = cxor.FindFeatureReferenceMisorientationsFilter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/CellFeatureData/AvgQuats"),
    #cell_feature_attribute_matrix_path=cx.DataPath("fw-ar-IF1-aptr-corr"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/CellEnsembleData/CrystalStructures"),
    feature_avg_misorientations_array_name=("FeatureAvgMisorientations"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/FeatureIds"),
    feature_reference_misorientations_array_name=("FeatureReferenceMisorientations"),
    #g_beuclidean_distances_array_path=cx.DataPath("fw-ar-IF1-aptr-corr"),
    quats_array_path=cx.DataPath("fw-ar-IF1-aptr-corr/Cell Data/Quats"),
    reference_orientation=0
)

#Filter 21

result = cx.FeatureDataCSVWriterFilter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=cx.DataPath("fw-ar-IF1-aptr-corr/CellFeatureData"),
    delimiter_choice_int=2,
    feature_data_file=cx.DataPath("Data/Output/fw-ar-IF1-avtr12-corr/FeatureData.csv"),
    write_neighborlist_data=False,
    write_num_features_line=True
)

#Filter 22

result = cx.CalculateArrayHistogramFilter.execute(
    data_structure=data_structure,
    #data_group_name: DataPath = ...,
    histogram_suffix=("Histogram"),
    #max_range: float = ...,
    #min_range: float = ...,
    new_data_group=True,
    new_data_group_name=cx.DataPath("Histograms"),
    number_of_bins=256,
    selected_array_paths=[cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData/EquivalentDiameters")],
    user_defined_range=False
)

#Filter 23

result = cx.WriteASCIIDataFilter.execute(
    data_structure=data_structure,
    delimiter=2,
    #file_extension: str = ...,
    includes=1,
    #max_val_per_line: int = ...,
    #output_dir=cx.DataPath(""),
    output_path=cx.DataPath("Data/Output/fw-ar-IF1-avtr12-corr/EqDiamHistogram.csv"),
    output_style=1,
    selected_data_array_paths=[cx.DataPath("fw-ar-IF1-avtr12-corr/Histograms/EquivalentDiameters Histogram")]
)

#Filter 24

output_file_path = "Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")