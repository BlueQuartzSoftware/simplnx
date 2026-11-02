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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/materials/microstructure_grayscale.png'),
    image_data_array_name='Microstructure Intensity',
    image_data_type_index=0,
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_geometry_path=nx.DataPath('Microstructure'),
    spacing=[1.0, 1.0, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Normalize Image Filter
result = nximgproc.NormalizeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Microstructure Intensity'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    output_array_name='Normalized Microstructure',
)
simplnx_utilities.check_filter_result(nximgproc.NormalizeImageFilter, result)

# Filter 3: Gradient Anisotropic Diffusion Image Filter
result = nximgproc.GradientAnisotropicDiffusionImageFilter.execute(
    data_structure=data_structure,
    conductance_parameter=3.0,
    conductance_scaling_update_interval=1,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Normalized Microstructure'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    number_of_iterations=3,
    output_array_name='Diffusion Smoothed',
    time_step=0.05,
)
simplnx_utilities.check_filter_result(nximgproc.GradientAnisotropicDiffusionImageFilter, result)

# Filter 4: Gradient Magnitude Image Filter
result = nximgproc.GradientMagnitudeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Diffusion Smoothed'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    output_array_name='Boundary Strength',
    use_image_spacing=True,
)
simplnx_utilities.check_filter_result(nximgproc.GradientMagnitudeImageFilter, result)

# Filter 5: H Maxima Image Filter
result = nximgproc.HMaximaImageFilter.execute(
    data_structure=data_structure,
    height=0.5,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Diffusion Smoothed'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    output_array_name='Prominent Peaks',
)
simplnx_utilities.check_filter_result(nximgproc.HMaximaImageFilter, result)

# Filter 6: Regional Maxima Image Filter
result = nximgproc.RegionalMaximaImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    flat_is_maxima=True,
    foreground_value=1.0,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Diffusion Smoothed'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    output_array_name='Regional Maxima',
)
simplnx_utilities.check_filter_result(nximgproc.RegionalMaximaImageFilter, result)

# Filter 7: Valued Regional Maxima Image Filter
result = nximgproc.ValuedRegionalMaximaImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Diffusion Smoothed'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    output_array_name='Valued Maxima',
)
simplnx_utilities.check_filter_result(nximgproc.ValuedRegionalMaximaImageFilter, result)

# Filter 8: Valued Regional Minima Image Filter
result = nximgproc.ValuedRegionalMinimaImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Diffusion Smoothed'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    output_array_name='Valued Minima',
)
simplnx_utilities.check_filter_result(nximgproc.ValuedRegionalMinimaImageFilter, result)

# Filter 9: Morphological Watershed Image Filter
result = nximgproc.MorphologicalWatershedImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Microstructure/Cell Data/Boundary Strength'),
    input_image_geometry_path=nx.DataPath('Microstructure'),
    level=0.3,
    mark_watershed_line=True,
    output_array_name='Watershed Labels',
)
simplnx_utilities.check_filter_result(nximgproc.MorphologicalWatershedImageFilter, result)

# Filter 10: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/05_Microstructure_Watershed/microstructure_watershed.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
