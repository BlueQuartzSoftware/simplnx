import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ReadCtfDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("EBSD Scan Data"),
    cell_ensemble_attribute_matrix_name=("Phase Data"),
    data_container_name=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    degrees_to_radians=True,
    edax_hexagonal_alignment=True,
    input_file=cx.DataPath("Data/Textured_Copper/Cugrid_after 2nd_15kv_2kx_2.ctf")
)

#Filter 2

result = cx.RotateSampleRefFrameFilter.execute(
    data_structure=data_structure,
    #created_image_geometry=cx.DataPath("DataContainer/"),
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix: List[List[float]] = ...,
    rotation_representation=("Axis Angle"),
    selected_image_geometry=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2")
)

#Filter 3

result = cx.CropImageGeometry.execute(
    data_structure=data_structure,
    #cell_feature_attribute_matrix=cx.DataPath("DataContainer"),
    #created_image_geometry=cx.DataPath("DataContainer"),
    #feature_ids=cx.DataPath("DataContainer"),
    max_voxel=[488, 0, 0],
    min_voxel=[549, 399, 0],
    remove_original_geometry=True,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    update_origin=True
)

#Filter 4

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Error"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the MultiThresholdObjects")

#Filter 5

result = cxor.GenerateIPFColorsFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPF_Exposed_001"),
    cell_phases_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Phases"),
    crystal_structures_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/CrystalStructures"),
    good_voxels_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_good_voxels=True
)

#Filter 6

result = cxitk.ITKImageWriter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/TexturedCopper/IPF_Unexposed.png"),
    image_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/IPF_Exposed_001"),
    image_geom_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    index_offset=0,
    plane=0
)

#Filter 7

result = cxor.WritePoleFigureFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    cell_phases_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    crystal_structures_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/CrystalStructures"),
    generation_algorithm=0,
    good_voxels_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Mask"),
    image_geometry_path=cx.DataPath("PoleFigure"),
    image_layout=2,
    image_prefix=("Cugrid_after 2nd_15kv_2kx_2_Unexposed_"),
    image_size=1024,
    lambert_size=64,
    material_name_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/MaterialName"),
    num_colors=32,
    output_path=cx.DataPath("Data/Output/TexturedCopper"),
    save_as_image_geometry=True,
    title=("Cugrid_after 2nd_15kv_2kx_2 Unexposed"),
    use_good_voxels=True,
    write_image_to_disk=True
)