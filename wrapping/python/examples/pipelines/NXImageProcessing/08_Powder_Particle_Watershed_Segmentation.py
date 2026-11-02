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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/materials/powder_particles.png'),
    image_data_array_name='Particle Intensity',
    image_data_type_index=0,
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_geometry_path=nx.DataPath('Powder Particles'),
    spacing=[1.0, 1.0, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Discrete Gaussian Image Filter
result = nximgproc.DiscreteGaussianImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Particle Intensity'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    maximum_error=[0.01, 0.01, 0.01],
    maximum_kernel_width=16,
    output_array_name='Smoothed Particles',
    use_image_spacing=True,
    variance=[1.0, 1.0, 0.0],
)
simplnx_utilities.check_filter_result(nximgproc.DiscreteGaussianImageFilter, result)

# Filter 3: Gradient Magnitude Recursive Gaussian Image Filter
result = nximgproc.GradientMagnitudeRecursiveGaussianImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Smoothed Particles'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    normalize_across_scale=False,
    output_array_name='Particle Gradient',
    sigma=1.0,
)
simplnx_utilities.check_filter_result(nximgproc.GradientMagnitudeRecursiveGaussianImageFilter, result)

# Filter 4: H Minima Image Filter
result = nximgproc.HMinimaImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    height=5.0,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Particle Gradient'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    output_array_name='Suppressed Minima',
)
simplnx_utilities.check_filter_result(nximgproc.HMinimaImageFilter, result)

# Filter 5: Regional Minima Image Filter
result = nximgproc.RegionalMinimaImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    flat_is_minima=True,
    foreground_value=1.0,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Suppressed Minima'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    output_array_name='Marker Mask',
)
simplnx_utilities.check_filter_result(nximgproc.RegionalMinimaImageFilter, result)

# Filter 6: Connected Component Image Filter
result = nximgproc.ConnectedComponentImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Marker Mask'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    output_array_name='Marker Labels',
)
simplnx_utilities.check_filter_result(nximgproc.ConnectedComponentImageFilter, result)

# Filter 7: Morphological Watershed From Markers Image Filter
result = nximgproc.MorphologicalWatershedFromMarkersImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Particle Gradient'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    mark_watershed_line=True,
    marker_image_data_path=nx.DataPath('Powder Particles/Cell Data/Marker Labels'),
    output_array_name='Particle Labels',
)
simplnx_utilities.check_filter_result(nximgproc.MorphologicalWatershedFromMarkersImageFilter, result)

# Filter 8: Label Contour Image Filter
result = nximgproc.LabelContourImageFilter.execute(
    data_structure=data_structure,
    background_value=0.0,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Powder Particles/Cell Data/Particle Labels'),
    input_image_geometry_path=nx.DataPath('Powder Particles'),
    output_array_name='Particle Boundaries',
)
simplnx_utilities.check_filter_result(nximgproc.LabelContourImageFilter, result)

# Filter 9: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/04_Powder_Watershed/powder_particles.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
