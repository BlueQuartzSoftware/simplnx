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
    output_geometry_path=nx.DataPath('Binary Mask'),
    spacing=[1.0, 1.0, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Binary Dilate Image Filter
result = nximgproc.BinaryDilateImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    boundary_to_foreground=False,
    foreground_value=1.0,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    output_array_name='Dilated Mask',
)
simplnx_utilities.check_filter_result(nximgproc.BinaryDilateImageFilter, result)

# Filter 3: Binary Erode Image Filter
result = nximgproc.BinaryErodeImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    boundary_to_foreground=True,
    foreground_value=1.0,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    output_array_name='Eroded Mask',
)
simplnx_utilities.check_filter_result(nximgproc.BinaryErodeImageFilter, result)

# Filter 4: Binary Morphological Opening Image Filter
result = nximgproc.BinaryMorphologicalOpeningImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    foreground_value=1.0,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    output_array_name='Opened Mask',
)
simplnx_utilities.check_filter_result(nximgproc.BinaryMorphologicalOpeningImageFilter, result)

# Filter 5: Binary Morphological Closing Image Filter
result = nximgproc.BinaryMorphologicalClosingImageFilter.execute(
    data_structure=data_structure,
    foreground_value=1.0,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    output_array_name='Closed Mask',
    safe_border=True,
)
simplnx_utilities.check_filter_result(nximgproc.BinaryMorphologicalClosingImageFilter, result)

# Filter 6: Binary Opening By Reconstruction Image Filter
result = nximgproc.BinaryOpeningByReconstructionImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    foreground_value=1.0,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    output_array_name='Reconstructed Opening',
)
simplnx_utilities.check_filter_result(nximgproc.BinaryOpeningByReconstructionImageFilter, result)

# Filter 7: Binary Thinning Image Filter
result = nximgproc.BinaryThinningImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    output_array_name='Skeleton',
)
simplnx_utilities.check_filter_result(nximgproc.BinaryThinningImageFilter, result)

# Filter 8: Not Image Filter
result = nximgproc.NotImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    output_array_name='Inverse Mask',
)
simplnx_utilities.check_filter_result(nximgproc.NotImageFilter, result)

# Filter 9: Connected Component Image Filter
result = nximgproc.ConnectedComponentImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Mask'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    output_array_name='Object Labels',
)
simplnx_utilities.check_filter_result(nximgproc.ConnectedComponentImageFilter, result)

# Filter 10: Dilate Object Morphology Image Filter
result = nximgproc.DilateObjectMorphologyImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Object Labels'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    object_value=1.0,
    output_array_name='Dilated Label 1',
)
simplnx_utilities.check_filter_result(nximgproc.DilateObjectMorphologyImageFilter, result)

# Filter 11: Erode Object Morphology Image Filter
result = nximgproc.ErodeObjectMorphologyImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    input_image_data_path=nx.DataPath('Binary Mask/Cell Data/Object Labels'),
    input_image_geometry_path=nx.DataPath('Binary Mask'),
    kernel_radius=[2, 2, 0],
    kernel_type_index=1,
    object_value=1.0,
    output_array_name='Eroded Label 1',
)
simplnx_utilities.check_filter_result(nximgproc.ErodeObjectMorphologyImageFilter, result)

# Filter 12: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/06_Binary_Mask/binary_mask_repair.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
