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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/materials/edge_input.png'),
    image_data_array_name='Input Intensity',
    image_data_type_index=0,
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_geometry_path=nx.DataPath('Edge Image'),
    spacing=[1.0, 1.0, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Normalize Image Filter
result = nximgproc.NormalizeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Input Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    output_array_name='Normalized Intensity',
)
simplnx_utilities.check_filter_result(nximgproc.NormalizeImageFilter, result)

# Filter 3: Curvature Flow Image Filter
result = nximgproc.CurvatureFlowImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    number_of_iterations=3,
    output_array_name='Curvature Flow',
    time_step=0.05,
)
simplnx_utilities.check_filter_result(nximgproc.CurvatureFlowImageFilter, result)

# Filter 4: Min Max Curvature Flow Image Filter
result = nximgproc.MinMaxCurvatureFlowImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    number_of_iterations=3,
    output_array_name='Min-Max Curvature Flow',
    stencil_radius=2,
    time_step=0.05,
)
simplnx_utilities.check_filter_result(nximgproc.MinMaxCurvatureFlowImageFilter, result)

# Filter 5: Smoothing Recursive Gaussian Image Filter
result = nximgproc.SmoothingRecursiveGaussianImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    normalize_across_scale=False,
    output_array_name='Recursive Gaussian',
    sigma=[1.0, 1.0, 0.0],
)
simplnx_utilities.check_filter_result(nximgproc.SmoothingRecursiveGaussianImageFilter, result)

# Filter 6: Discrete Gaussian Image Filter
result = nximgproc.DiscreteGaussianImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    maximum_error=[0.01, 0.01, 0.01],
    maximum_kernel_width=16,
    output_array_name='Discrete Gaussian',
    use_image_spacing=False,
    variance=[1.0, 1.0, 0.0],
)
simplnx_utilities.check_filter_result(nximgproc.DiscreteGaussianImageFilter, result)

# Filter 7: Gradient Magnitude Image Filter
result = nximgproc.GradientMagnitudeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    output_array_name='Finite-Difference Gradient',
    use_image_spacing=True,
)
simplnx_utilities.check_filter_result(nximgproc.GradientMagnitudeImageFilter, result)

# Filter 8: Gradient Magnitude Recursive Gaussian Image Filter
result = nximgproc.GradientMagnitudeRecursiveGaussianImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    normalize_across_scale=False,
    output_array_name='Gaussian Gradient',
    sigma=1.0,
)
simplnx_utilities.check_filter_result(nximgproc.GradientMagnitudeRecursiveGaussianImageFilter, result)

# Filter 9: Laplacian Recursive Gaussian Image Filter
result = nximgproc.LaplacianRecursiveGaussianImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Normalized Intensity'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    normalize_across_scale=False,
    output_array_name='Laplacian Response',
    sigma=1.0,
)
simplnx_utilities.check_filter_result(nximgproc.LaplacianRecursiveGaussianImageFilter, result)

# Filter 10: Zero Crossing Image Filter
result = nximgproc.ZeroCrossingImageFilter.execute(
    data_structure=data_structure,
    background_value=0,
    foreground_value=1,
    input_image_data_path=nx.DataPath('Edge Image/Cell Data/Laplacian Response'),
    input_image_geometry_path=nx.DataPath('Edge Image'),
    output_array_name='Laplacian Zero Crossings',
)
simplnx_utilities.check_filter_result(nximgproc.ZeroCrossingImageFilter, result)

# Filter 11: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/09_Denoising_Edges/denoising_and_edges.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
