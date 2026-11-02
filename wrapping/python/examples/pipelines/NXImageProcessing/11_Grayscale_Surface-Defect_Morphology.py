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
    file_name=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/materials/grayscale_defects.png'),
    image_data_array_name='Defect Intensity',
    image_data_type_index=0,
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    origin_spacing_processing_index=1,
    output_geometry_path=nx.DataPath('Grayscale Defects'),
    spacing=[1.0, 1.0, 1.0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadImageFilter, result)

# Filter 2: Grayscale Dilate Image Filter
result = nximgproc.GrayscaleDilateImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Local Maximum',
)
simplnx_utilities.check_filter_result(nximgproc.GrayscaleDilateImageFilter, result)

# Filter 3: Grayscale Erode Image Filter
result = nximgproc.GrayscaleErodeImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Local Minimum',
)
simplnx_utilities.check_filter_result(nximgproc.GrayscaleErodeImageFilter, result)

# Filter 4: Grayscale Fillhole Image Filter
result = nximgproc.GrayscaleFillholeImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    output_array_name='Filled Dark Holes',
)
simplnx_utilities.check_filter_result(nximgproc.GrayscaleFillholeImageFilter, result)

# Filter 5: Grayscale Grind Peak Image Filter
result = nximgproc.GrayscaleGrindPeakImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    output_array_name='Ground Peaks',
)
simplnx_utilities.check_filter_result(nximgproc.GrayscaleGrindPeakImageFilter, result)

# Filter 6: Grayscale Morphological Closing Image Filter
result = nximgproc.GrayscaleMorphologicalClosingImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Grayscale Closed',
    safe_border=True,
)
simplnx_utilities.check_filter_result(nximgproc.GrayscaleMorphologicalClosingImageFilter, result)

# Filter 7: Grayscale Morphological Opening Image Filter
result = nximgproc.GrayscaleMorphologicalOpeningImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Grayscale Opened',
    safe_border=True,
)
simplnx_utilities.check_filter_result(nximgproc.GrayscaleMorphologicalOpeningImageFilter, result)

# Filter 8: Opening By Reconstruction Image Filter
result = nximgproc.OpeningByReconstructionImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Opening Reconstruction',
    preserve_intensities=True,
)
simplnx_utilities.check_filter_result(nximgproc.OpeningByReconstructionImageFilter, result)

# Filter 9: Closing By Reconstruction Image Filter
result = nximgproc.ClosingByReconstructionImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Closing Reconstruction',
    preserve_intensities=True,
)
simplnx_utilities.check_filter_result(nximgproc.ClosingByReconstructionImageFilter, result)

# Filter 10: Black Top Hat Image Filter
result = nximgproc.BlackTopHatImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Dark Defect Response',
    safe_border=True,
)
simplnx_utilities.check_filter_result(nximgproc.BlackTopHatImageFilter, result)

# Filter 11: White Top Hat Image Filter
result = nximgproc.WhiteTopHatImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Bright Defect Response',
    safe_border=True,
)
simplnx_utilities.check_filter_result(nximgproc.WhiteTopHatImageFilter, result)

# Filter 12: Morphological Gradient Image Filter
result = nximgproc.MorphologicalGradientImageFilter.execute(
    data_structure=data_structure,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    kernel_radius=[3, 3, 0],
    kernel_type_index=1,
    output_array_name='Morphological Gradient',
)
simplnx_utilities.check_filter_result(nximgproc.MorphologicalGradientImageFilter, result)

# Filter 13: H Convex Image Filter
result = nximgproc.HConvexImageFilter.execute(
    data_structure=data_structure,
    fully_connected=True,
    height=20.0,
    input_image_data_path=nx.DataPath('Grayscale Defects/Cell Data/Defect Intensity'),
    input_image_geometry_path=nx.DataPath('Grayscale Defects'),
    output_array_name='Prominent Bright Defects',
)
simplnx_utilities.check_filter_result(nximgproc.HConvexImageFilter, result)

# Filter 14: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/07_Grayscale_Morphology/grayscale_defects.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
