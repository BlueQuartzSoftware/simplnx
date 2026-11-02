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
    output_geometry_path=nx.DataPath('Projection Volume'),
    output_transformation_matrix_path=nx.DataPath('MHA Image/TransformationMatrix'),
    save_image_transformation=False,
    transpose_transform_matrix=False,
)
simplnx_utilities.check_filter_result(nximgproc.ReadMhaFileFilter, result)

# Filter 2: Binary Threshold Image Filter
result = nximgproc.BinaryThresholdImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    inside_value=1,
    lower_threshold=0.0,
    output_array_name='Pore Mask',
    outside_value=0,
    upper_threshold=10000.0,
)
simplnx_utilities.check_filter_result(nximgproc.BinaryThresholdImageFilter, result)

# Filter 3: Binary Projection Image Filter
result = nximgproc.BinaryProjectionImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    foreground_value=1.0,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/Pore Mask'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Pore Occupancy',
    output_image_geometry_name='Binary Pore Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.BinaryProjectionImageFilter, result)

# Filter 4: Maximum Projection Image Filter
result = nximgproc.MaximumProjectionImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Maximum Intensity',
    output_image_geometry_name='Maximum XCT Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.MaximumProjectionImageFilter, result)

# Filter 5: Mean Projection Image Filter
result = nximgproc.MeanProjectionImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Mean Intensity',
    output_image_geometry_name='Mean XCT Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.MeanProjectionImageFilter, result)

# Filter 6: Median Projection Image Filter
result = nximgproc.MedianProjectionImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Median Intensity',
    output_image_geometry_name='Median XCT Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.MedianProjectionImageFilter, result)

# Filter 7: Minimum Projection Image Filter
result = nximgproc.MinimumProjectionImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Minimum Intensity',
    output_image_geometry_name='Minimum XCT Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.MinimumProjectionImageFilter, result)

# Filter 8: Standard Deviation Projection Image Filter
result = nximgproc.StandardDeviationProjectionImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Intensity Deviation',
    output_image_geometry_name='Deviation XCT Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.StandardDeviationProjectionImageFilter, result)

# Filter 9: Sum Projection Image Filter
result = nximgproc.SumProjectionImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Projection Volume/Cell Data/XCT Intensity'),
    input_image_geometry_path=nx.DataPath('Projection Volume'),
    output_array_name='Integrated Intensity',
    output_image_geometry_name='Sum XCT Projection',
    projection_dimension=2,
    remove_original_geometry=False,
)
simplnx_utilities.check_filter_result(nximgproc.SumProjectionImageFilter, result)

# Filter 10: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/10_Projection_QA/projection_summaries.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
