import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = cxor.ReadH5EbsdFilter()

h5ebsdParameter = cxor.ReadH5EbsdFileParameter.ValueType()
h5ebsdParameter.euler_representation=0
h5ebsdParameter.end_slice=117
h5ebsdParameter.selected_array_names=["Confidence Index", "EulerAngles", "Fit", "Image Quality", "Phases", "SEM Signal", "X Position", "Y Position"]
h5ebsdParameter.input_file_path=str(nxtest.get_data_directory() / "Output/Reconstruction/Small_IN100.h5ebsd")
h5ebsdParameter.start_slice=1
h5ebsdParameter.use_recommended_transform=True

# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="CellData",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    created_image_geometry_path =nx.DataPath("DataContainer"),
    read_h5_ebsd_object=h5ebsdParameter
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
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

# Execute Filter with Parameters
result = nx.MultiThresholdObjects.execute(data_structure=data_structure,
                                            array_thresholds_object=threshold_set,
                                            created_data_array_name = "Mask",
                                            created_mask_type = nx.DataType.boolean,
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = cxor.ConvertOrientations()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=nx.DataPath("DataContainer/CellData/EulerAngles"),
    input_type=0,
    output_orientation_array_name="Quats",
    output_type=2
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
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
    selected_image_geometry_path =nx.DataPath("DataContainer"),
    cell_mask_array_path=nx.DataPath("DataContainer/CellData/Mask"),
    misorientation_tolerance=5.0,
    cell_quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    randomize_features=True,
    use_mask=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = nx.FindLargestCrossSectionsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=nx.DataPath("DataContainer/CellFeatureData"),
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    image_geometry_path=nx.DataPath("DataContainer"),
    largest_cross_sections_array_name="LargestCrossSections",
    plane=0
)
nxtest.check_filter_result(nx_filter, result)


# Filter 6
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Define output file path
output_file_path = nxtest.get_data_directory() / "Output/Examples/SmallIN100_LargestCrossSections.dream3d"
# Execute WriteDREAM3DFilter with Parameters
result = nx_filter.execute(data_structure=data_structure, 
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
