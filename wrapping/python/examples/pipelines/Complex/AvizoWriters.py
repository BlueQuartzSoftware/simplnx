import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = nx.ReadDREAM3DFilter()
# Set import parameters
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = nxtest.GetDataDirectory() + "/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, import_file_data=import_data)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.WriteAvizoUniformCoordinateFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=nx.DataPath("DataContainer"),
    output_file=nxtest.GetDataDirectory() + "/Output/Examples/SmallIN100_AvizoUniform.am",
    units="meters",
    write_binary_file=False
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.WriteAvizoUniformCoordinateFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=nx.DataPath("DataContainer"),
    output_file=nxtest.GetDataDirectory() + "/Output/Examples/SmallIN100_AvizoRectilinear.am",
    units="meters",
    write_binary_file=False
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")