import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadCtfDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    degrees_to_radians=True,
    edax_hexagonal_alignment=True,
    input_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/T12-MAI-2010/fw-ar-IF1-avtr12-corr.ctf"
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
filter = cx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix=[]
    rotation_representation=0,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr")
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
filter = cx.MultiThresholdObjects()
# Set Threshold Conditions
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Error")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

# Create a Threshold Set
threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]

# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="ThresholdArray",
    created_mask_type=cx.DataType.boolean
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
filter = cxor.ConvertOrientations()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    input_type=0,
    output_orientation_array_name="Quats",
    output_type=2
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
filter = cx.ReplaceElementAttributesWithNeighborValuesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    comparison_data_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Error"),
    loop=False,
    min_confidence=0.0,
    selected_comparison=0,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr")
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
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name="IPF_001",
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
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
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_001.png",
    image_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_001"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane=0
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
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name="IPF_010",
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    #mask_array_path=cx.DataPath("")
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 9
# Instantiate Filter
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_010.png",
    image_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_010"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane=0
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 10
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/EulerAngles"),
    cell_ipf_colors_array_name="IPF_100",
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    reference_dir=[1.0, 0.0, 0.0],
    use_mask=False
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 11
# Instantiate Filter
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr_100.png",
    image_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/IPF_100"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    index_offset=0,
    plane=0
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 12
# Instantiate Filter
filter = cxor.EBSDSegmentFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    cell_feature_attribute_matrix_name="CellFeatureData",
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_ids_array_name="FeatureIds",
    grid_geometry_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    mask_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/ThresholdArray"),
    misorientation_tolerance=5.0,
    quats_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    randomize_features=True,
    use_mask=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 13
# Instantiate Filter
filter = cx.FillBadDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    min_allowed_defect_size=10,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    store_as_new_phase=False
    # cell_phases_array_path and ignored_data_array_paths parameters are not used in this context
)
if len(result.warnings) !=0:
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
    feature_attribute_matrix=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    geometry_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    num_elements_path="NumElements",
    save_element_sizes=False,
    volumes_path="Volumes"
)
if len(result.warnings) !=0:
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
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    image_geom_path=cx.DataPath("fw-ar-IF1-avtr12-corr"),
    min_allowed_features_size=5,
    num_cells_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData/NumElements")
    # feature_phases_path and phase_number parameters are not used in this context
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 16
# Instantiate Filter
filter = cxor.FindAvgOrientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path="AvgEulerAngles",
    avg_quats_array_path="AvgQuats",
    cell_feature_attribute_matrix=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData"),
    cell_feature_ids_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    cell_quats_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 17
# Instantiate Filter
filter = cxor.FindKernelAvgMisorientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    kernel_average_misorientations_array_name="KernelAverageMisorientations",
    kernel_size=[1, 1, 1],
    quats_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    selected_image_geometry_path=cx.DataPath("fw-ar-IF1-avtr12-corr")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 18
# Instantiate Filter
filter = cx.FindFeatureCentroidsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    centroids_array_path="Centroids",
    feature_attribute_matrix=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData"),
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 19
# Instantiate Filter
filter = cx.FindEuclideanDistMapFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    calc_manhattan_dist=True,
    do_boundaries=True,
    do_quad_points=False,
    do_triple_lines=False,
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    g_bdistances_array_name="GBManhattanDistances",
    save_nearest_neighbors=False,
    selected_image_geometry=cx.DataPath("fw-ar-IF1-avtr12-corr")
    # Parameters for nearest_neighbors_array_name, q_pdistances_array_name, and t_jdistances_array_name are not used
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 20
# Instantiate Filter
filter = cxor.FindFeatureReferenceMisorientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData/AvgQuats"),
    cell_phases_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Phases"),
    crystal_structures_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellEnsembleData/CrystalStructures"),
    feature_avg_misorientations_array_name="FeatureAvgMisorientations",
    feature_ids_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/FeatureIds"),
    feature_reference_misorientations_array_name="FeatureReferenceMisorientations",
    quats_array_path=cx.DataPath("fw-ar-IF1-avtr12-corr/Cell Data/Quats"),
    reference_orientation=0
    # Parameters for cell_feature_attribute_matrix_path and g_beuclidean_distances_array_path are not used
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 21
# Instantiate Filter
filter = cx.WriteFeatureDataCSVFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData"),
    delimiter_choice_int=2,
    feature_data_file="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/fw-ar-IF1-avtr12-corr/FeatureData.csv",
    write_neighborlist_data=False,
    write_num_features_line=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 22
# Instantiate Filter
filter = cx.CalculateArrayHistogramFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    histogram_suffix=" Histogram",
    new_data_group=True,
    new_data_group_name=cx.DataPath("fw-ar-IF1-avtr12-corr/Histograms"),
    number_of_bins=256,
    selected_array_paths=[cx.DataPath("fw-ar-IF1-avtr12-corr/CellFeatureData/EquivalentDiameters")],
    user_defined_range=False
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 23
# Instantiate Filter
filter = cx.WriteASCIIDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    delimiter=2,
    output_path="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/fw-ar-IF1-avtr12-corr/EqDiamHistogram.csv",
    output_style=1,
    selected_data_array_paths=[cx.DataPath("fw-ar-IF1-avtr12-corr/Histograms/EquivalentDiameters Histogram")]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 24
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path="C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Output/fw-ar-IF1-avtr12-corr/fw-ar-IF1-avtr12-corr.dream3d",
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