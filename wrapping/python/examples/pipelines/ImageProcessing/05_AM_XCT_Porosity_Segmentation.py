import simplnx as nx
import imageprocessing as nximgproc
import simplnx_utilities
import simplnx_test_dirs as nxtest

import numpy as np

data_structure = nx.DataStructure()

# Filter 1: Read MHA/MetaImage File
crop_values_1 = nx.CropGeometryParameter.ValueType()
crop_values_1.type = nx.CropGeometryParameter.TypeEnum.NoCropping
crop_values_1.is_2d = False
crop_values_1.crop_x = True
crop_values_1.crop_y = True
crop_values_1.crop_z = True
crop_values_1.x_bound_voxels = [0, 0]
crop_values_1.y_bound_voxels = [0, 0]
crop_values_1.z_bound_voxels = [0, 0]
crop_values_1.x_bound_physical = [0.0, 0.0]
crop_values_1.y_bound_physical = [0.0, 0.0]
crop_values_1.z_bound_physical = [0.0, 0.0]
result = nximgproc.ReadMhaFileFilter.execute(
    data_structure=data_structure,
    apply_image_transformation=False,
    cell_attribute_matrix_name='Cell Data',
    cropping_options_index=crop_values_1,
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/xct_porosity.mha'),
    image_data_array_name='XCT Intensity',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('AM XCT'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 2: Intensity Windowing Image Filter
result = nximgproc.IntensityWindowingImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    output_array_name='Windowed Intensity',
    output_maximum=65535.0,
    output_minimum=0.0,
    window_maximum=50000.0,
    window_minimum=0.0,
)
simplnx_utilities.check_filter_result(nximgproc.IntensityWindowingImageFilter, result)

# Filter 3: Normalize Image Filter
result = nximgproc.NormalizeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Windowed Intensity'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    output_array_name='Normalized Intensity',
)
simplnx_utilities.check_filter_result(nximgproc.NormalizeImageFilter, result)

# Filter 4: Curvature Anisotropic Diffusion Image Filter
result = nximgproc.CurvatureAnisotropicDiffusionImageFilter.execute(
    data_structure=data_structure,
    conductance_parameter=3.0,
    conductance_scaling_update_interval=1,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    number_of_iterations=3,
    output_array_name='Denoised Intensity',
    time_step=0.001,
)
simplnx_utilities.check_filter_result(nximgproc.CurvatureAnisotropicDiffusionImageFilter, result)

# Filter 5: Binary Threshold Image Filter
result = nximgproc.BinaryThresholdImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Denoised Intensity'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    inside_value=1,
    lower_threshold=-20.0,
    output_array_name='Pore Mask',
    outside_value=0,
    upper_threshold=-2.0,
)
simplnx_utilities.check_filter_result(nximgproc.BinaryThresholdImageFilter, result)

# Filter 6: Binary Morphological Closing Image Filter
result = nximgproc.BinaryMorphologicalClosingImageFilter.execute(
    data_structure=data_structure,
    foreground_value=1.0,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Pore Mask'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    kernel_radius=[1, 1, 1],
    kernel_type_index=1,
    output_array_name='Closed Pore Mask',
    safe_border=True,
)
simplnx_utilities.check_filter_result(nximgproc.BinaryMorphologicalClosingImageFilter, result)

# Filter 7: Connected Component Image Filter
result = nximgproc.ConnectedComponentImageFilter.execute(
    data_structure=data_structure,
    fully_connected=False,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Closed Pore Mask'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    output_array_name='Pore Labels',
)
simplnx_utilities.check_filter_result(nximgproc.ConnectedComponentImageFilter, result)

# Filter 8: Relabel Component Image Filter
result = nximgproc.RelabelComponentImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Pore Labels'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    minimum_object_size=8,
    output_array_name='Relabeled Pores',
    sort_by_object_size=True,
)
simplnx_utilities.check_filter_result(nximgproc.RelabelComponentImageFilter, result)

# Filter 9: Binary Contour Image Filter
result = nximgproc.BinaryContourImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    foreground_value=1.0,
    fully_connected=False,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Closed Pore Mask'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    output_array_name='Pore Contours',
)
simplnx_utilities.check_filter_result(nximgproc.BinaryContourImageFilter, result)

# Filter 10: Signed Maurer Distance Map Image Filter
result = nximgproc.SignedMaurerDistanceMapImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    input_image_data_path=nx.DataPath('AM XCT/Cell Data/Closed Pore Mask'),
    input_image_geometry_path=nx.DataPath('AM XCT'),
    inside_is_positive=False,
    output_array_name='Signed Pore Distance',
    squared_distance=False,
    use_image_spacing=True,
)
simplnx_utilities.check_filter_result(nximgproc.SignedMaurerDistanceMapImageFilter, result)

# Filter 11: Write Image
result = nximgproc.WriteImageFilter.execute(
    data_structure=data_structure,
    add_scale_bar=False,
    create_color_table=False,
    file_name='Data/Output/ImageProcessing_Examples/01_AM_XCT/pore_contours.tif',
    flip_mode_index=0,
    image_array_path=nx.DataPath('AM XCT/Cell Data/Pore Contours'),
    index_offset=0,
    input_image_geometry_path=nx.DataPath('AM XCT'),
    invalid_color_value=[0, 0, 0],
    leading_digit_character='0',
    mask_array_path=nx.DataPath(''),
    plane_index=0,
    selected_preset='Black-Body Radiation',
    total_index_digits=3,
    use_mask=False,
)
simplnx_utilities.check_filter_result(nximgproc.WriteImageFilter, result)

# Filter 12: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/01_AM_XCT/am_xct_porosity.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
