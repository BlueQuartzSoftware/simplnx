import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = nxtest.GetDataDirectory() + "/Output/Statistics/SmallIN100_Morph.dream3d"
import_data.data_paths = None
# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure, import_file_data=import_data)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.DeleteData()
# Execute Filter With Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    removed_data_path=[nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
                       nx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles")]

)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate and Execute Filter
# Note: This filter might need additional parameters depending on the intended data removal.
nx_filter = nx.DeleteData()
result = nx_filter.execute(
    data_structure=data_structure
    # removed_data_path: List[DataPath] = ...  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = cxor.FindAvgOrientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_euler_angles_array_path="AvgEulerAngles",
    avg_quats_array_path="AvgQuats",
    cell_feature_attribute_matrix=nx.DataPath("DataContainer/CellFeatureData"),
    cell_feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    cell_quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = cxor.FindMisorientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    find_avg_misors=False,
    misorientation_list_array_name="MisorientationList",
    neighbor_list_array_path=nx.DataPath("DataContainer/CellFeatureData/NeighborhoodList")
    # avg_misorientations_array_name: str = ...,  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = cxor.FindSchmidsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
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
nxtest.check_filter_result(nx_filter, result)

# Filter 7
# Instantiate Filter
nx_filter = cxor.FindFeatureReferenceMisorientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_avg_misorientations_array_name="FeatureAvgMisorientations",
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    feature_reference_misorientations_array_name="FeatureReferenceMisorientations",
    quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    reference_orientation=0
    # cell_feature_attribute_matrix_path=nx.DataPath("DataContainer/"),  # Not currently part of the code
    # g_beuclidean_distances_array_path=nx.DataPath("DataContainer/"),  # Not currently part of the code
)
nxtest.check_filter_result(nx_filter, result)

# Filter 8
# Instantiate Filter
nx_filter = cxor.FindKernelAvgMisorientationsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_phases_array_path=nx.DataPath("DataContainer/CellData/Phases"),
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_ids_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    kernel_average_misorientations_array_name="KernelAverageMisorientations",
    kernel_size=[1, 1, 1],
    quats_array_path=nx.DataPath("DataContainer/CellData/Quats"),
    selected_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 9
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path =  nxtest.GetDataDirectory() + "/Output/Statistics/SmallIN100_CrystalStats.dream3d"
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure, 
    export_file_path=output_file_path, 
    write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(import_data.file_path)
# *****************************************************************************

print("===> Pipeline Complete")