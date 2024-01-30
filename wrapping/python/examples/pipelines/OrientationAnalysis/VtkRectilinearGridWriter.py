import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1

import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = str(nxtest.get_data_directory() / "Output/Reconstruction/SmallIN100_Final.dream3d")
import_data.data_paths = None

# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        import_file_data=import_data)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.DeleteData()
# Execute Filter
result = nx_filter.execute(
    data_structure=data_structure,
    removed_data_path=[nx.DataPath("DataContainer/CellEnsembleData/MaterialName"),
                       nx.DataPath("MergeTwins SeedValue")]
)

# Filter 3
# Instantiate Filter
nx_filter = nx.WriteVtkRectilinearGridFilter()
output_file_path = nxtest.get_data_directory() / "Output/Examples/SmallIN100_Final.vtk"
# Execute Filter
result = nx_filter.execute(
    data_structure=data_structure,
    image_geometry_path=nx.DataPath("DataContainer"),
    output_file=output_file_path,
    write_binary_file=True,
    selected_data_array_paths=[
 nx.DataPath("DataContainer/CellData/Confidence Index"), 
 nx.DataPath("DataContainer/CellData/EulerAngles"),
 nx.DataPath("DataContainer/CellData/FeatureIds"),
 nx.DataPath("DataContainer/CellData/Fit"),
 nx.DataPath("DataContainer/CellData/IPFColors"),
 nx.DataPath("DataContainer/CellData/Image Quality"),
 nx.DataPath("DataContainer/CellData/Mask"),
 nx.DataPath("DataContainer/CellData/ParentIds"),
 nx.DataPath("DataContainer/CellData/Phases"),
 nx.DataPath("DataContainer/CellData/Quats"),
 nx.DataPath("DataContainer/CellData/SEM Signal"),
 nx.DataPath("DataContainer/CellData/X Position"),
 nx.DataPath("DataContainer/CellData/Y Position")]
)
nxtest.check_filter_result(nx_filter, result)


# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************

print("===> Pipeline Complete")
