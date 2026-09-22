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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/calibration/transmission.mha'),
    image_data_array_name='Transmission',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('Transmission Calibration'),
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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/calibration/residual.mha'),
    image_data_array_name='Residual',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('Residual Calibration'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 3: Read MHA/MetaImage File
crop_values_3 = nx.CropGeometryParameter.ValueType()
crop_values_3.type = nx.CropGeometryParameter.TypeEnum.NoCropping
crop_values_3.is_2d = False
crop_values_3.crop_x = True
crop_values_3.crop_y = True
crop_values_3.crop_z = True
crop_values_3.x_bound_voxels = [0, 0]
crop_values_3.y_bound_voxels = [0, 0]
crop_values_3.z_bound_voxels = [0, 0]
crop_values_3.x_bound_physical = [0.0, 0.0]
crop_values_3.y_bound_physical = [0.0, 0.0]
crop_values_3.z_bound_physical = [0.0, 0.0]
result = nximgproc.ReadMhaFileFilter.execute(
    data_structure=data_structure,
    apply_image_transformation=False,
    cell_attribute_matrix_name='Cell Data',
    cropping_options_index=crop_values_3,
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/am/calibration/mask.mha'),
    image_data_array_name='Mask',
    interpolation_type_index=0,
    output_geometry_path=nx.DataPath('Calibration Mask'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 4: Abs Image Filter
result = nximgproc.AbsImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Residual Calibration/Cell Data/Residual'),
    input_image_geometry_path=nx.DataPath('Residual Calibration'),
    output_array_name='Absolute Residual',
)
simplnx_utilities.check_filter_result(nximgproc.AbsImageFilter, result)

# Filter 5: Bounded Reciprocal Image Filter
result = nximgproc.BoundedReciprocalImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Bounded Reciprocal',
)
simplnx_utilities.check_filter_result(nximgproc.BoundedReciprocalImageFilter, result)

# Filter 6: Exp Image Filter
result = nximgproc.ExpImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Exponential Response',
)
simplnx_utilities.check_filter_result(nximgproc.ExpImageFilter, result)

# Filter 7: Exp Negative Image Filter
result = nximgproc.ExpNegativeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Negative Exponential',
)
simplnx_utilities.check_filter_result(nximgproc.ExpNegativeImageFilter, result)

# Filter 8: Log Image Filter
result = nximgproc.LogImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Natural Log Transmission',
)
simplnx_utilities.check_filter_result(nximgproc.LogImageFilter, result)

# Filter 9: Log10 Image Filter
result = nximgproc.Log10ImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Base-10 Log Transmission',
)
simplnx_utilities.check_filter_result(nximgproc.Log10ImageFilter, result)

# Filter 10: Sqrt Image Filter
result = nximgproc.SqrtImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Square Root Transmission',
)
simplnx_utilities.check_filter_result(nximgproc.SqrtImageFilter, result)

# Filter 11: Square Image Filter
result = nximgproc.SquareImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Squared Transmission',
)
simplnx_utilities.check_filter_result(nximgproc.SquareImageFilter, result)

# Filter 12: Normalize To Constant Image Filter
result = nximgproc.NormalizeToConstantImageFilter.execute(
    data_structure=data_structure,
    constant=100.0,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Constant-Normalized Transmission',
)
simplnx_utilities.check_filter_result(nximgproc.NormalizeToConstantImageFilter, result)

# Filter 13: Rescale Intensity Image Filter
result = nximgproc.RescaleIntensityImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    output_array_name='Display Range',
    output_maximum=255.0,
    output_minimum=0.0,
)
simplnx_utilities.check_filter_result(nximgproc.RescaleIntensityImageFilter, result)

# Filter 14: Invert Intensity Image Filter
result = nximgproc.InvertIntensityImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    maximum=1.0,
    output_array_name='Inverted Transmission',
)
simplnx_utilities.check_filter_result(nximgproc.InvertIntensityImageFilter, result)

# Filter 15: Threshold Image Filter
result = nximgproc.ThresholdImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    lower=0.1,
    output_array_name='Valid Transmission Range',
    outside_value=0.0,
    upper=0.9,
)
simplnx_utilities.check_filter_result(nximgproc.ThresholdImageFilter, result)

# Filter 16: Mask Image Filter
result = nximgproc.MaskImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Transmission Calibration/Cell Data/Transmission'),
    input_image_geometry_path=nx.DataPath('Transmission Calibration'),
    mask_image_data_path=nx.DataPath('Calibration Mask/Cell Data/Mask'),
    output_array_name='Masked Transmission',
    outside_value=0.0,
)
simplnx_utilities.check_filter_result(nximgproc.MaskImageFilter, result)

# Filter 17: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/11_Radiography_Calibration/radiography_calibration.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
