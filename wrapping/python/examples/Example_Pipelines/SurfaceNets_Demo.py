import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxitk.ITKImportImageStack.execute(
    data_structure=data_structure,
    cell_data_name="Optical Data",
    image_data_array_path="ImageData",
    image_geometry_path=cx.DataPath("RoboMet.3D Image Stack"),
    image_transform_choice=0,
    input_file_list_info=cx.DataPath("Data/Porosity_Image"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)

#Filter 2

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["RoboMet.3D Imgage Stack/Optical Data/ImageData"]),
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = cx.DataType.boolean

result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                        array_thresholds=threshold_set,
                                        created_data_path="Mask",
                                        created_mask_type=cx.DataType.boolean)

result = cx.MultiThresholdObjects.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=9,
    #custom_false_value: float = ...,
    #custom_true_value: float = ...,
    use_custom_false_value=False,
    use_custom_true_value=False
)

#Filter 3

result = cx.ScalarSegmentFeaturesFilter.execute(
    data_structure=data_structure,
    active_array_path="Active",
    cell_feature_group_path="Pore Data",
    feature_ids_path="FeatureIds",
    grid_geometry_path=cx.DataPath("RoboMet.3D Image Stack"),
    input_array_path="RoboMet.3D Image Stack/Optical Data/Image Data",
    mask_path=cx.DataPath("RoboMet.3D Image Stack/Optical Data/Mask"),
    randomize_features=False,
    scalar_tolerance=0,
    use_mask=False
)

#Filter 4

result = cx.SurfaceNetsFilter.execute(
    data_structure=data_structure,
    apply_smoothing=True,
    face_data_group_name="Face Data",
    face_feature_attribute_matrix_name="Face Feature Data",
    face_labels_array_name="FaceLabels",
    feature_ids_path=cx.DataPath("RoboMet.3D Image Stack/Optical Data/FeatureIds"),
    grid_geometry_data_path=cx.DataPath("RoboMet.3D Image Stack"),
    max_distance_from_voxel=1.0,
    node_types_array_name="NodeTypes",
    relaxation_factor=0.5,
    #selected_data_array_paths: List[DataPath] = ...,
    smoothing_iterations=20,
    triangle_geometry_name=cx.DataPath("TriangleDataContainer"),
    vertex_data_group_name="Vertex Data"
)

#Filter 5

output_file_path = "Data/Output/SurfaceMesh/SurfaceNets_Demo.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")
