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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/scientific/sample.mha'),
    image_data_array_name='MHA Data',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('MHA Volume'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 2: Read NIfTI File (Version 1)
crop_values_2 = nx.CropGeometryParameter.ValueType()
crop_values_2.type = nx.CropGeometryParameter.TypeEnum.NoCropping
crop_values_2.is_2d = False
crop_values_2.crop_x = True
crop_values_2.crop_y = True
crop_values_2.crop_z = True
crop_values_2.x_bound_voxels = [0, 0]
crop_values_2.y_bound_voxels = [0, 0]
crop_values_2.z_bound_voxels = [0, 0]
crop_values_2.x_bound_physical = [0.0, 0.0]
crop_values_2.y_bound_physical = [0.0, 0.0]
crop_values_2.z_bound_physical = [0.0, 0.0]
result = nximgproc.ReadNIfTIFileFilter.execute(
    data_structure=data_structure,
    apply_scaling_transform=True,
    cell_attribute_matrix_name='Cell Data',
    cropping_options_index=crop_values_2,
    image_data_array_name='NIfTI Data',
    input_file_path=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/scientific/sample.nii.gz'),
    output_image_geometry_path=nx.DataPath('NIfTI Volume'),
    use_affine_if_present=True,
)
simplnx_utilities.check_filter_result(nximgproc.ReadNIfTIFileFilter, result)

# Filter 3: Write Image
result = nximgproc.WriteImageFilter.execute(
    data_structure=data_structure,
    add_scale_bar=False,
    create_color_table=False,
    file_name='Data/Output/ImageProcessing_Examples/13_Scientific_IO/mha_round_trip.mha',
    flip_mode_index=0,
    image_array_path=nx.DataPath('MHA Volume/Cell Data/MHA Data'),
    index_offset=0,
    input_image_geometry_path=nx.DataPath('MHA Volume'),
    invalid_color_value=[0, 0, 0],
    leading_digit_character='0',
    mask_array_path=nx.DataPath(''),
    plane_index=0,
    selected_preset='Black-Body Radiation',
    total_index_digits=3,
    use_mask=False,
)
simplnx_utilities.check_filter_result(nximgproc.WriteImageFilter, result)

# Filter 4: Write Image
result = nximgproc.WriteImageFilter.execute(
    data_structure=data_structure,
    add_scale_bar=False,
    create_color_table=False,
    file_name='Data/Output/ImageProcessing_Examples/13_Scientific_IO/nifti_slice.tif',
    flip_mode_index=0,
    image_array_path=nx.DataPath('NIfTI Volume/Cell Data/NIfTI Data'),
    index_offset=0,
    input_image_geometry_path=nx.DataPath('NIfTI Volume'),
    invalid_color_value=[0, 0, 0],
    leading_digit_character='0',
    mask_array_path=nx.DataPath(''),
    plane_index=0,
    selected_preset='Black-Body Radiation',
    total_index_digits=3,
    use_mask=False,
)
simplnx_utilities.check_filter_result(nximgproc.WriteImageFilter, result)

# Filter 5: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/13_Scientific_IO/scientific_volumes.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
