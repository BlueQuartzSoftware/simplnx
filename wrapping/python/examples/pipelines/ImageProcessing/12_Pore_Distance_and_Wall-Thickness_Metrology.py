import simplnx as nx
import imageprocessing as nximgproc
import simplnx_utilities
import simplnx_test_dirs as nxtest

import numpy as np


data_structure = nx.DataStructure()

# Filter 1: Read Image
crop_values_1 = nx.CropGeometryParameter.ValueType()
crop_values_1.type = nx.CropGeometryParameter.TypeEnum.NoCropping
crop_values_1.is_2d = True
crop_values_1.crop_x = True
crop_values_1.crop_y = True
crop_values_1.crop_z = True
crop_values_1.x_bound_voxels = [0, 0]
crop_values_1.y_bound_voxels = [0, 0]
crop_values_1.z_bound_voxels = [0, 0]
crop_values_1.x_bound_physical = [0.0, 0.0]
crop_values_1.y_bound_physical = [0.0, 0.0]
crop_values_1.z_bound_physical = [0.0, 0.0]
result = nximgproc.ReadImageFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name='Cell Data',
    center_origin=False,
    change_image_data_type=False,
    change_origin=False,
    change_spacing=False,
    cropping_options=crop_values_1,
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/materials/binary_mask.png'),
    image_data_array_name='Mask',
    image_data_type_index=0,
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_geometry_path=nx.DataPath('Distance Mask'),
    spacing=[1.0, 1.0, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Approximate Signed Distance Map Image Filter
result = nximgproc.ApproximateSignedDistanceMapImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Distance Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Distance Mask'),
    inside_value=1.0,
    output_array_name='Approximate Signed Distance',
    outside_value=0.0,
)
simplnx_utilities.check_filter_result(nximgproc.ApproximateSignedDistanceMapImageFilter, result)

# Filter 3: Danielsson Distance Map Image Filter
result = nximgproc.DanielssonDistanceMapImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Distance Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Distance Mask'),
    input_is_binary=True,
    output_array_name='Danielsson Distance',
    squared_distance=False,
    use_image_spacing=True,
)
simplnx_utilities.check_filter_result(nximgproc.DanielssonDistanceMapImageFilter, result)

# Filter 4: Signed Danielsson Distance Map Image Filter
result = nximgproc.SignedDanielssonDistanceMapImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Distance Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Distance Mask'),
    inside_is_positive=False,
    output_array_name='Signed Danielsson Distance',
    squared_distance=False,
    use_image_spacing=True,
)
simplnx_utilities.check_filter_result(nximgproc.SignedDanielssonDistanceMapImageFilter, result)

# Filter 5: Signed Maurer Distance Map Image Filter
result = nximgproc.SignedMaurerDistanceMapImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    input_image_data_path=nx.DataPath('Distance Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Distance Mask'),
    inside_is_positive=False,
    output_array_name='Signed Maurer Distance',
    squared_distance=False,
    use_image_spacing=True,
)
simplnx_utilities.check_filter_result(nximgproc.SignedMaurerDistanceMapImageFilter, result)

# Filter 6: Iso Contour Distance Image Filter
result = nximgproc.IsoContourDistanceImageFilter.execute(
    data_structure=data_structure,
    far_value=10.0,
    input_image_data_path=nx.DataPath('Distance Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Distance Mask'),
    level_set_value=0.5,
    output_array_name='Iso-Contour Distance',
)
simplnx_utilities.check_filter_result(nximgproc.IsoContourDistanceImageFilter, result)

# Filter 7: Zero Crossing Image Filter
result = nximgproc.ZeroCrossingImageFilter.execute(
    data_structure=data_structure,
    background_value=0,
    foreground_value=1,
    input_image_data_path=nx.DataPath('Distance Mask/Cell Data/Signed Maurer Distance'),
    input_image_geometry_path=nx.DataPath('Distance Mask'),
    output_array_name='Distance Zero Crossings',
)
simplnx_utilities.check_filter_result(nximgproc.ZeroCrossingImageFilter, result)

# Filter 8: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/08_Distance_Metrology/distance_metrology.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
