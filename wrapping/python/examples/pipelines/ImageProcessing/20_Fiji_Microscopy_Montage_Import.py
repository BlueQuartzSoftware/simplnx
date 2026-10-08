import simplnx as nx
import imageprocessing as nximgproc
import simplnx_utilities
import simplnx_test_dirs as nxtest

import numpy as np


data_structure = nx.DataStructure()

# Filter 1: Read Fiji Montage
result = nximgproc.ImportFijiMontageFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name='Cell Data',
    change_image_data_type=False,
    change_origin=False,
    color_weights=[0.2125, 0.7154, 0.0721],
    convert_to_gray_scale=False,
    data_container_path='Tile',
    data_group_name='Fiji Montage',
    image_data_array_name='Image',
    image_data_type_index=0,
    input_file=str(nxtest.get_data_directory() / 'ImageProcessing_Examples/io/fiji/TileConfiguration.registered.txt'),
    length_unit_index=6,
    origin=[0.0, 0.0, 0.0],
    parent_data_group=True,
)
simplnx_utilities.check_filter_result(nximgproc.ImportFijiMontageFilter, result)

# Filter 2: Write DREAM3D-NX File
result = nx.WriteDREAM3DFilter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path='Data/Output/ImageProcessing_Examples/16_Fiji_Montage/fiji_montage.dream3d',
    use_compression=True,
    write_xdmf_file=False,
)
simplnx_utilities.check_filter_result(nx.WriteDREAM3DFilter, result)

print("===> Pipeline Complete")
