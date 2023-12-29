import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadCtfDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("EBSD Scan Data"),
    cell_ensemble_attribute_matrix_name=("Phase Data"),
    data_container_name=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    degrees_to_radians=True,
    edax_hexagonal_alignment=True,
    input_file=nxtest.GetDataDirectory() + "/Textured_Copper/Cugrid_after 2nd_15kv_2kx_2.ctf"
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix=
    rotation_representation=0,
    selected_image_geometry=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2")
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.CropImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    max_voxel=[549, 399, 0],
    min_voxel=[488, 0, 0],
    remove_original_geometry=True,
    renumber_features=False,
    selected_image_geometry=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2")
)
nxtest.check_filter_result(filter, result)

# Filter 4
# Instantiate Filter
threshold_1 = nx.ArrayThreshold()
threshold_1.array_path = nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Error")
threshold_1.comparison = nx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = nx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
result = nx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=nx.DataType.boolean)

nxtest.check_filter_result(filter, result)

# Filter 5
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPF_Unexposed_001"),
    cell_phases_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Phases"),
    crystal_structures_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/CrystalStructures"),
    mask_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=True
)
nxtest.check_filter_result(filter, result)

# Filter 6
# Instantiate Filter
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name=nxtest.GetDataDirectory() + "/Output/TexturedCopper/IPF_Unexposed.png",
    image_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/IPF_Unexposed_001"),
    image_geom_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    index_offset=0,
    plane=0
)
nxtest.check_filter_result(filter, result)

# Filter 7
# Instantiate Filter
filter = cxor.WritePoleFigureFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    cell_phases_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Phases"),
    crystal_structures_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/CrystalStructures"),
    generation_algorithm=0,
    mask_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Mask"),
    image_geometry_path=nx.DataPath("PoleFigure"),
    image_layout=2,
    image_prefix="Cugrid_after 2nd_15kv_2kx_2_Unexposed_",
    image_size=1024,
    lambert_size=64,
    material_name_array_path=nx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/MaterialName"),
    num_colors=32,
    output_path=nxtest.GetDataDirectory() + "/Output/TexturedCopper",
    save_as_image_geometry=True,
    title="Cugrid_after 2nd_15kv_2kx_2 Unexposed",
    use_mask=True,
    write_image_to_disk=True
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")