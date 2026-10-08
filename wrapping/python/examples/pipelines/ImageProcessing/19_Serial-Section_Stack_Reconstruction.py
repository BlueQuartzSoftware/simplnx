import simplnx as nx
import imageprocessing as nximgproc
import simplnx_utilities
import simplnx_test_dirs as nxtest

import numpy as np


data_structure = nx.DataStructure()

# Filter 1: Read Images [3D Stack]
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
file_list_1 = nx.GeneratedFileListParameter.ValueType()
file_list_1.input_path = str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/serial_stack')
file_list_1.ordering = nx.GeneratedFileListParameter.Ordering.LowToHigh
file_list_1.file_prefix = 'section_'
file_list_1.file_suffix = ''
file_list_1.file_extension = '.tif'
file_list_1.start_index = 0
file_list_1.end_index = 7
file_list_1.increment_index = 1
file_list_1.padding_digits = 3
result = nximgproc.ReadImageStackFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name='Cell Data',
    change_image_data_type=False,
    change_origin=False,
    change_spacing=True,
    color_weights=[0.2125, 0.7154, 0.0721],
    convert_to_gray_scale=True,
    cropping_options=crop_values_1,
    exact_xy_dimensions=[256, 256],
    image_data_array_name='Section Intensity',
    image_data_type_index=0,
    image_transform_index=0,
    input_file_list_object=file_list_1,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_image_geometry_path=nx.DataPath('Serial Sections'),
    resample_images_index=0,
    scaling=100.0,
    spacing=[0.5, 0.5, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageStackFilter, result)

# Filter 2: Write Image
result = nximgproc.WriteImageFilter.execute(
    data_structure=data_structure,
    add_scale_bar=False,
    create_color_table=False,
    file_name=str(nxtest.get_data_directory() / 'Output/ImageProcessing_Examples/15_Serial_Sections/section_preview.tif'),
    flip_mode_index=0,
    image_array_path=nx.DataPath('Serial Sections/Cell Data/Section Intensity'),
    index_offset=0,
    input_image_geometry_path=nx.DataPath('Serial Sections'),
    invalid_color_value=[0, 0, 0],
    leading_digit_character='0',
    mask_array_path=nx.DataPath(''),
    plane_index=0,
    selected_preset='Black-Body Radiation',
    total_index_digits=3,
    use_mask=False,
)
simplnx_utilities.check_filter_result(nximgproc.WriteImageFilter, result)

# Filter 3: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/15_Serial_Sections/serial_sections.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
