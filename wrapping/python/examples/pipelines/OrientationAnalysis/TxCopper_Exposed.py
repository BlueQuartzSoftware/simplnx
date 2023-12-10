import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadCtfDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("EBSD Scan Data"),
    cell_ensemble_attribute_matrix_name=("Phase Data"),
    data_container_name=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    degrees_to_radians=True,
    edax_hexagonal_alignment=True,
    input_file=cxtest.GetDataDirectory() + "Data/Textured_Copper/Cugrid_after 2nd_15kv_2kx_2.ctf"
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    remove_original_geometry=True,
    rotate_slice_by_slice=False,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix=
    rotation_representation=0,
    selected_image_geometry=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.CropImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    max_voxel=[460, 399, 0],
    min_voxel=[0, 0, 0],
    remove_original_geometry=True,
    renumber_features=False,
    selected_image_geometry=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    update_origin=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Error")
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.Equal
threshold_1.value = 0.0

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)

if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    cell_ipf_colors_array_name=("IPF_Exposed_001"),
    cell_phases_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Phases"),
    crystal_structures_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/CrystalStructures"),
    mask_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Mask"),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name=cxtest.GetDataDirectory() + "Data/Output/TexturedCopper/IPF_Exposed.png",
    image_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/IPF_Exposed_001"),
    image_geom_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2"),
    index_offset=0,
    plane=0
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cxor.WritePoleFigureFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/EulerAngles"),
    cell_phases_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Phases"),
    crystal_structures_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/CrystalStructures"),
    generation_algorithm=0,
    mask_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/EBSD Scan Data/Mask"),
    image_geometry_path=cx.DataPath("PoleFigure"),
    image_layout=2,
    image_prefix="Cugrid_after 2nd_15kv_2kx_2_Exposed_",
    image_size=1024,
    lambert_size=64,
    material_name_array_path=cx.DataPath("Cugrid_after 2nd_15kv_2kx_2/Phase Data/MaterialName"),
    num_colors=32,
    output_path=cxtest.GetDataDirectory() + "Data/Output/TexturedCopper",
    save_as_image_geometry=True,
    title="Cugrid_after 2nd_15kv_2kx_2 Exposed",
    use_mask=True,
    write_image_to_disk=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")