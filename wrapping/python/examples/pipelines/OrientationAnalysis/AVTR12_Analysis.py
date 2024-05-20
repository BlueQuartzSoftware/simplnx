import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = cxor.ReadCtfDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    output_image_geometry_path =nx.DataPath("fw-ar-IF1-avtr12-corr"),
    degrees_to_radians=True,
    edax_hexagonal_alignment=True,
    input_file=nxtest.get_data_directory() / "T12-MAI-2010/fw-ar-IF1-avtr12-corr.ctf"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    output_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis_angle=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix=[]
    rotation_representation_index=0,
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.MultiThresholdObjectsFilter()
# Set Threshold Conditions
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Error")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

# Create a Threshold Set
threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds_object=threshold_set,
    output_data_array_name="ThresholdArray",
    created_mask_type=nx.DataType.boolean
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = cxor.ConvertOrientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    input_representation_index=0,
    output_orientation_array_name="Quats",
    output_representation_index=2
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.ReplaceElementAttributesWithNeighborValuesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    comparison_data_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Error"),
    loop=False,
    min_confidence=0.0,
    comparison_index=0,
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = cxor.ComputeIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name="IPF_001",
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 7
# Instantiate Filter
nx_filter = cxitk.ITKImageWriterFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    file_name=nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_001.png",
    image_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_001"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane_index=0
)
nxtest.check_filter_result(nx_filter, result)

# Filter 8
# Instantiate Filter
nx_filter = cxor.ComputeIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name="IPF_010",
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    #mask_array_path=nx.DataPath("")
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 9
# Instantiate Filter
nx_filter = cxitk.ITKImageWriterFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    file_name=nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_010.png",
    image_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_010"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane_index=0
)
nxtest.check_filter_result(nx_filter, result)

# Filter 10
# Instantiate Filter
nx_filter = cxor.ComputeIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name="IPF_100",
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    reference_dir=[1.0, 0.0, 0.0],
    use_mask=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 11
# Instantiate Filter
nx_filter = cxitk.ITKImageWriterFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    file_name=nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_100.png",
    image_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_100"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane_index=0
)
nxtest.check_filter_result(nx_filter, result)

# Filter 12
# Instantiate Filter
nx_filter = cxor.EBSDSegmentFeaturesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    cell_feature_attribute_matrix_name="Cell Feature Data",
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_ids_array_name="FeatureIds",
    input_image_geometry_path =nx.DataPath("fw-ar-IF1-avtr12-corr"),
    cell_mask_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/ThresholdArray"),
    misorientation_tolerance=5.0,
    cell_quats_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    randomize_features=True,
    use_mask=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 13
# Instantiate Filter
nx_filter = nx.FillBadDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    min_allowed_defect_size=10,
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    store_as_new_phase=False
    # cell_phases_array_path and ignored_data_array_paths parameters are not used in this context
)
nxtest.check_filter_result(nx_filter, result)

# Filter 14
# Instantiate Filter
nx_filter = nx.ComputeFeatureSizesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    equivalent_diameters_name="EquivalentDiameters",
    feature_attribute_matrix_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data"),
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    num_elements_name="NumElements",
    save_element_sizes=False,
    volumes_name="Volumes"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 15
# Instantiate Filter
nx_filter = nx.RequireMinimumSizeFeaturesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    apply_single_phase=False,
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr"),
    min_allowed_features_size=5,
    num_cells_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data/NumElements")
    # feature_phases_path and phase_number parameters are not used in this context
)
nxtest.check_filter_result(nx_filter, result)

# Filter 16
# Instantiate Filter
nx_filter = cxor.ComputeAvgOrientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_name="AvgEulerAngles",
    avg_quats_array_name="AvgQuats",
    cell_feature_attribute_matrix_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data"),
    cell_feature_ids_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    cell_quats_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 17
# Instantiate Filter
nx_filter = cxor.ComputeKernelAvgMisorientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    kernel_average_misorientations_array_name="KernelAverageMisorientations",
    kernel_size=[1, 1, 1],
    quats_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 18
# Instantiate Filter
nx_filter = nx.ComputeFeatureCentroidsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    centroids_array_name="Centroids",
    feature_attribute_matrix_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data"),
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 19
# Instantiate Filter
nx_filter = nx.ComputeEuclideanDistMapFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    calc_manhattan_dist=True,
    do_boundaries=True,
    do_quad_points=False,
    do_triple_lines=False,
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    g_bdistances_array_name="GBManhattanDistances",
    save_nearest_neighbors=False,
    input_image_geometry_path=nx.DataPath("fw-ar-IF1-avtr12-corr")
    # Parameters for nearest_neighbors_array_name, q_pdistances_array_name, and t_jdistances_array_name are not used
)
nxtest.check_filter_result(nx_filter, result)

# Filter 20
# Instantiate Filter
nx_filter = cxor.ComputeFeatureReferenceMisorientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data/AvgQuats"),
    cell_phases_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_avg_misorientations_array_name="FeatureAvgMisorientations",
    feature_ids_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    feature_reference_misorientations_array_name="FeatureReferenceMisorientations",
    quats_array_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    reference_orientation_index=0
    # Parameters for cell_feature_attribute_matrix_path and g_beuclidean_distances_array_path are not used
)
nxtest.check_filter_result(nx_filter, result)

# Filter 21
# Instantiate Filter
nx_filter = nx.WriteFeatureDataCSVFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data"),
    delimiter_index=2,
    feature_data_file=nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/FeatureData.csv",
    write_neighborlist_data=False,
    write_num_features_line=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 22
# Instantiate Filter
nx_filter = nx.ComputeArrayHistogramFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    histogram_suffix=" Histogram",
    create_new_data_group=True,
    new_data_group_path=nx.DataPath("fw-ar-IF1-avtr12-corr/Histograms"),
    number_of_bins=256,
    selected_array_paths=[nx.DataPath("fw-ar-IF1-avtr12-corr/Cell Feature Data/EquivalentDiameters")],
    user_defined_range=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 23
# Instantiate Filter
nx_filter = nx.WriteASCIIDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    delimiter_index=2,
    output_path=nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/EqDiamHistogram.csv",
    output_style_index=1,
    input_data_array_paths=[nx.DataPath("fw-ar-IF1-avtr12-corr/Histograms/EquivalentDiameters Histogram")]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 24
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
output_file_path = nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr.dream3d"
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
nxtest.cleanup_test_file(output_file_path)
nxtest.cleanup_test_dir(nxtest.get_data_directory() / "Output/fw-ar-IF1-avtr12-corr/")
# *****************************************************************************


print("===> Pipeline Complete")
