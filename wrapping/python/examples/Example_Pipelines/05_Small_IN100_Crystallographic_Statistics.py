import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_Morph.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = cx.ImportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ImportDREAM3DFilter filter")

# Filter 2
# Instantiate and Execute Filter
# Note: This filter might need additional parameters depending on the intended data removal.
filter = cx.DeleteData()
result = filter.execute(
    data_structure=data_structure
    # removed_data_path: List[DataPath] = ...  # Not currently part of the code
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cxor.FindAvgOrientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path="AvgEulerAngles",
    avg_quats_array_path="AvgQuats",
    cell_feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    cell_feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    cell_quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the FindAvgOrientationsFilter")

# Filter 4
# Instantiate Filter
filter = cxor.FindMisorientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    find_avg_misors=False,
    misorientation_list_array_name="MisorientationList",
    neighbor_list_array_path=cx.DataPath("DataContainer/CellFeatureData/NeighborhoodList")
    # avg_misorientations_array_name: str = ...,  # Not currently part of the code
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cxor.FindSchmidsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    loading_direction=[1.0, 1.0, 1.0],
    override_system=False,
    poles_array_name="Poles",
    schmids_array_name="Schmids",
    slip_systems_array_name="SlipSystems",
    store_angle_components=False
    # lambdas_array_name: str = ...,  # Not currently part of the code
    # phis_array_name: str = ...,  # Not currently part of the code
    # slip_direction: List[float] = ...,  # Not currently part of the code
    # slip_plane: List[float] = ...,  # Not currently part of the code
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cxor.FindFeatureReferenceMisorientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_avg_misorientations_array_name="FeatureAvgMisorientations",
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_reference_misorientations_array_name="FeatureReferenceMisorientations",
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    reference_orientation=0
    # cell_feature_attribute_matrix_path=cx.DataPath("DataContainer/"),  # Not currently part of the code
    # g_beuclidean_distances_array_path=cx.DataPath("DataContainer/"),  # Not currently part of the code
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cxor.FindKernelAvgMisorientationsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    kernel_average_misorientations_array_name="KernelAverageMisorientations",
    kernel_size=[1, 1, 1],
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=cx.DataPath("DataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Set Output File Path
output_file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure, 
    export_file_path=output_file_path, 
    write_xdmf_file=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ExportDREAM3DFilter")