import simplnx as nx
import imageprocessing as nximgproc
import simplnx_utilities
import simplnx_test_dirs as nxtest

import numpy as np


data_structure = nx.DataStructure()

# Filter 1: Read North Star Imaging CT (.nsihdr/.nsidat)
result = nximgproc.ReadBinaryCTNorthstarFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name='Cell Data',
    density_array_name='Density',
    end_voxel_coord=[1, 1, 1],
    import_subvolume=False,
    input_header_file=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/northstar/sample.nsihdr'),
    input_image_geometry_path=nx.DataPath('North Star XCT'),
    length_unit_index=6,
    start_voxel_coord=[0, 0, 0],
)
simplnx_utilities.check_filter_result(nximgproc.ReadBinaryCTNorthstarFilter, result)

# Filter 2: Read Volume Graphics File (.vgi/.vol)
result = nximgproc.ReadVolumeGraphicsFileFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name='Cell Data',
    density_array_name='Density',
    output_image_geometry_path=nx.DataPath('Volume Graphics XCT'),
    vg_header_file=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/volume_graphics/VolumeGraphicsTest.vgi'),
)
simplnx_utilities.check_filter_result(nximgproc.ReadVolumeGraphicsFileFilter, result)

# Filter 3: Read Zeiss TXM/TXRM Files
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
result = nximgproc.ReadZeissTxmFileFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name='Cell Data',
    cropping_options_index=crop_values_1,
    ct_data_array_name='CT Data',
    input_file_path=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/zeiss/sample.txm'),
    output_image_geometry_path=nx.DataPath('Zeiss XCT'),
)
simplnx_utilities.check_filter_result(nximgproc.ReadZeissTxmFileFilter, result)

# Filter 4: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/14_Industrial_XCT/industrial_xct_imports.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
