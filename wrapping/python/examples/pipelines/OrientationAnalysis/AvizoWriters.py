import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Set import parameters
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = str(nxtest.get_data_directory() / "Output/Reconstruction/SmallIN100_Final.dream3d")
import_data.data_paths = None
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure, import_data_object=import_data)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.WriteAvizoUniformCoordinateFilter()
output_file_path = nxtest.get_data_directory() / "Output/AzizoWriter/SmallIN100_AvizoUniform.am"
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=nx.DataPath("DataContainer"),
    output_file=output_file_path,
    units="meters",
    write_binary_file=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.WriteAvizoUniformCoordinateFilter()
output_file_path = nxtest.get_data_directory() / "Output/AzizoWriter/SmallIN100_AvizoRectilinear.am"

# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=nx.DataPath("DataContainer"),
    output_file=output_file_path,
    units="meters",
    write_binary_file=False
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_dir(nxtest.get_data_directory() / "Output/AzizoWriter")
# *****************************************************************************

print("===> Pipeline Complete")
