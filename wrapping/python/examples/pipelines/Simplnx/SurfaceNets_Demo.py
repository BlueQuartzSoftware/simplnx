import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1

generated_file_list_value = nx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = str(nxtest.get_data_directory() / "Porosity_Image")
generated_file_list_value.ordering = nx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2

# Instantiate Filter
nx_filter = cxitk.ITKImportImageStackFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Optical Data",
    image_data_array_name="Image Data",
    output_image_geometry_path=nx.DataPath("RoboMet.3D Image Stack"),
    image_transform_index=0,
    input_file_list_object =generated_file_list_value,
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate ArrayThreshold objects
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("RoboMet.3D Image Stack/Optical Data/Image Data")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = nx.DataType.boolean

# Instantiate Filter
nx_filter = nx.MultiThresholdObjectsFilter()
# Execute filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_thresholds_object=threshold_set,
    output_data_array_name="Mask",
    created_mask_type=nx.DataType.boolean
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.ScalarSegmentFeaturesFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    active_array_name="Active",
    cell_feature_group_name="Pore Data",
    feature_ids_name="FeatureIds",
    input_image_geometry_path =nx.DataPath("RoboMet.3D Image Stack"),
    input_array_path=nx.DataPath("RoboMet.3D Image Stack/Optical Data/Image Data"),
    mask_path=nx.DataPath("RoboMet.3D Image Stack/Optical Data/Mask"),
    randomize_features=False,
    scalar_tolerance=0,
    use_mask=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.SurfaceNetsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    apply_smoothing=True,
    face_data_group_name="Face Data",
    face_feature_attribute_matrix_name="Face Feature Data",
    face_labels_array_name="FaceLabels",
    feature_ids_path=nx.DataPath("RoboMet.3D Image Stack/Optical Data/FeatureIds"),
    input_grid_geometry_path=nx.DataPath("RoboMet.3D Image Stack"),
    max_distance_from_voxel=1.0,
    node_types_array_name="NodeTypes",
    relaxation_factor=0.5,
    #input_data_array_paths: List[DataPath] = ...,
    smoothing_iterations=20,
    output_triangle_geometry_path=nx.DataPath("TriangleDataContainer"),
    vertex_data_group_name="Vertex Data"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
output_file_path = nxtest.get_data_directory() / "Output/SurfaceMesh/SurfaceNets_Demo.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)

nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************



print("===> Pipeline Complete")
