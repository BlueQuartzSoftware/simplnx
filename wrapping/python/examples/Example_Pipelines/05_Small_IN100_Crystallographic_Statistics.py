import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_Morph.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#Filter 2

result = cx.DeleteData.execute(
    data_structure=data_structure,
    #removed_data_path: List[DataPath] = ...
)

#Filter 3

result = cxor.FindAvgOrientationsFilter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path=("AvgEulerAngles"),
    avg_quats_array_path=("AvgQuats"),
    cell_feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    cell_feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    cell_quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
)
#Filter 4

result = cxor.FindMisorientationsFilter.execute(
    data_structure=data_structure,
    #avg_misorientations_array_name: str = ...,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    find_avg_misors=False,
    misorientation_list_array_name=("MisorientationList"),
    neighbor_list_array_path=cx.DataPath("DataContainer/CellFeatureData/NeighborhoodList"),
)
#Filter 5

result = cxor.FindSchmidsFilter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    #lambdas_array_name: str = ...,
    loading_direction=[1.0, 1.0, 1.0],
    override_system=False,
    #phis_array_name: str = ...,
    poles_array_name=("Poles"),
    schmids_array_name=("Schmids"),
    #slip_direction: List[float] = ...,
    #slip_plane: List[float] = ...,
    slip_systems_array_name=("SlipSystems"),
    store_angle_components=False,
)
#Filter 6

result = cxor.FindFeatureReferenceMisorientationsFilter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    #cell_feature_attribute_matrix_path=cx.DataPath("DataContainer/"),
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_avg_misorientations_array_name=("FeatureAvgMisorientations"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_reference_misorientations_array_name=("FeatureReferenceMisorientations"),
    g_beuclidean_distances_array_path=cx.DataPath("DataContainer/"),
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    reference_orientation=0,
)
#Filter 7

result = cxor.FindKernelAvgMisorientationsFilter.execute(
    data_structure=data_structure,
    cell_phases_array_path=cx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    kernel_average_misorientations_array_name=("KernelAverageMisorientations"),
    kernel_size=[1, 1, 1],
    quats_array_path=cx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=cx.DataPath("DataContainer"),
)
#Filter 8

output_file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")