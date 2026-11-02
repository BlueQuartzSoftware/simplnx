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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/calibration/phase_radians.mha'),
    image_data_array_name='Phase Radians',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('Phase Field'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 2: Read MHA/MetaImage File
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
result = nximgproc.ReadMhaFileFilter.execute(
    data_structure=data_structure,
    apply_image_transformation=False,
    cell_attribute_matrix_name='Cell Data',
    cropping_options_index=crop_values_2,
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/calibration/normalized_response.mha'),
    image_data_array_name='Normalized Response',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('Normalized Response'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 3: Sin Image Filter
result = nximgproc.SinImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Phase Field/Cell Data/Phase Radians'),
    input_image_geometry_path=nx.DataPath('Phase Field'),
    output_array_name='Sine',
)
simplnx_utilities.check_filter_result(nximgproc.SinImageFilter, result)

# Filter 4: Cos Image Filter
result = nximgproc.CosImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Phase Field/Cell Data/Phase Radians'),
    input_image_geometry_path=nx.DataPath('Phase Field'),
    output_array_name='Cosine',
)
simplnx_utilities.check_filter_result(nximgproc.CosImageFilter, result)

# Filter 5: Tan Image Filter
result = nximgproc.TanImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Phase Field/Cell Data/Phase Radians'),
    input_image_geometry_path=nx.DataPath('Phase Field'),
    output_array_name='Tangent',
)
simplnx_utilities.check_filter_result(nximgproc.TanImageFilter, result)

# Filter 6: Asin Image Filter
result = nximgproc.AsinImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Normalized Response/Cell Data/Normalized Response'),
    input_image_geometry_path=nx.DataPath('Normalized Response'),
    output_array_name='Inverse Sine',
)
simplnx_utilities.check_filter_result(nximgproc.AsinImageFilter, result)

# Filter 7: Acos Image Filter
result = nximgproc.AcosImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Normalized Response/Cell Data/Normalized Response'),
    input_image_geometry_path=nx.DataPath('Normalized Response'),
    output_array_name='Inverse Cosine',
)
simplnx_utilities.check_filter_result(nximgproc.AcosImageFilter, result)

# Filter 8: Atan Image Filter
result = nximgproc.AtanImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Normalized Response/Cell Data/Normalized Response'),
    input_image_geometry_path=nx.DataPath('Normalized Response'),
    output_array_name='Inverse Tangent',
)
simplnx_utilities.check_filter_result(nximgproc.AtanImageFilter, result)

# Filter 9: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/12_Phase_Angles/phase_and_angle_transforms.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
