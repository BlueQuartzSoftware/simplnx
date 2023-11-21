import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1

generated_file_list_value = cx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-windows-AMD64/Data/Porosity_Image"
generated_file_list_value.ordering = cx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2

# Instantiate Filter
filter = cxitk.ITKImportImageStack()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="Optical Data",
    image_data_array_path="ImageData",
    image_geometry_path=cx.DataPath("RoboMet.3D Image Stack"),
    image_transform_choice=0,
    input_file_list_info=generated_file_list_value,
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate ArrayThreshold objects
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("RoboMet.3D Imgage Stack/Optical Data/ImageData")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

# Define ArrayThresholdSet object
threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
dt = cx.DataType.boolean

# Instantiate Filter
filter = cx.MultiThresholdObjects()
# Execute filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_thresholds=threshold_set,
    created_data_path="Mask",
    created_mask_type=cx.DataType.boolean
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.ScalarSegmentFeaturesFilter()
# Execute Filter with Parameters
result = filter.execute(
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
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.SurfaceNetsFilter()
# Execute Filter with Parameters
result = filter.execute(
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
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
output_file_path = "Data/Output/SurfaceMesh/SurfaceNets_Demo.dream3d"
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)

if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")