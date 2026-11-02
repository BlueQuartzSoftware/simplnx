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
    change_spacing=True,
    cropping_options=crop_values_1,
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/melt_pool_track.tif'),
    image_data_array_name='Thermal Intensity',
    image_data_type_index=0,
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_geometry_path=nx.DataPath('Melt Pool Image'),
    spacing=[0.0333, 0.0472, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Normalize Image Filter
result = nximgproc.NormalizeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Thermal Intensity'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    output_array_name='Normalized Intensity',
)
simplnx_utilities.check_filter_result(nximgproc.NormalizeImageFilter, result)

# Filter 3: Sigmoid Image Filter
result = nximgproc.SigmoidImageFilter.execute(
    data_structure=data_structure,
    alpha=-0.5,
    beta=0.0,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    output_array_name='Sigmoid Contrast',
    output_maximum=255.0,
    output_minimum=0.0,
)
simplnx_utilities.check_filter_result(nximgproc.SigmoidImageFilter, result)

# Filter 4: Otsu Multiple Thresholds Image Filter
result = nximgproc.OtsuMultipleThresholdsImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Sigmoid Contrast'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    label_offset=0,
    number_of_histogram_bins=128,
    number_of_thresholds=2,
    output_array_name='Otsu Regions',
    return_bin_midpoint=False,
    valley_emphasis=True,
)
simplnx_utilities.check_filter_result(nximgproc.OtsuMultipleThresholdsImageFilter, result)

# Filter 5: Double Threshold Image Filter
result = nximgproc.DoubleThresholdImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Thermal Intensity'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    inside_value=1,
    output_array_name='Track Mask',
    outside_value=0,
    threshold1=40.0,
    threshold2=90.0,
    threshold3=150.0,
    threshold4=255.0,
)
simplnx_utilities.check_filter_result(nximgproc.DoubleThresholdImageFilter, result)

# Filter 6: Connected Component Image Filter
result = nximgproc.ConnectedComponentImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Track Mask'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    output_array_name='Track Labels',
)
simplnx_utilities.check_filter_result(nximgproc.ConnectedComponentImageFilter, result)

# Filter 7: Relabel Component Image Filter
result = nximgproc.RelabelComponentImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Track Labels'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    minimum_object_size=8,
    output_array_name='Sorted Track Labels',
    sort_by_object_size=True,
)
simplnx_utilities.check_filter_result(nximgproc.RelabelComponentImageFilter, result)

# Filter 8: Label Contour Image Filter
result = nximgproc.LabelContourImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Melt Pool Image/Cell Data/Sorted Track Labels'),
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    output_array_name='Track Contours',
)
simplnx_utilities.check_filter_result(nximgproc.LabelContourImageFilter, result)

# Filter 9: Write Image
result = nximgproc.WriteImageFilter.execute(
    data_structure=data_structure,
    add_scale_bar=False,
    create_color_table=True,
    file_name='Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/track_contours.tif',
    flip_mode_index=0,
    image_array_path=nx.DataPath('Melt Pool Image/Cell Data/Track Contours'),
    index_offset=0,
    input_image_geometry_path=nx.DataPath('Melt Pool Image'),
    invalid_color_value=[0, 0, 0],
    leading_digit_character='0',
    mask_array_path=nx.DataPath(''),
    plane_index=0,
    selected_preset='Black-Body Radiation',
    total_index_digits=3,
    use_mask=False,
)
simplnx_utilities.check_filter_result(nximgproc.WriteImageFilter, result)

# Filter 10: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/03_AM_Melt_Pool/melt_pool_inspection.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
