import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1

import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = nxtest.GetDataDirectory() + "/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None

# Instantiate Filter
filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        import_file_data=import_data)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.DeleteData()
# Execute Filter
result = filter.execute(
    data_structure=data_structure,
    removed_data_path=[nx.DataPath("DataContainer/CellEnsembleData/MaterialName"),
                       nx.DataPath("MergeTwins SeedValue")]
)

# Filter 3
# Instantiate Filter
filter = nx.WriteVtkRectilinearGridFilter()
# Execute Filter
result = filter.execute(
    data_structure=data_structure,
    image_geometry_path=nx.DataPath("DataContainer"),
    output_file=nxtest.GetDataDirectory() + "/Output/Examples/SmallIN100_Final.vtk",
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
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")