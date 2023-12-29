import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Import Data

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
filter = nx.FindBoundaryCellsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    boundary_cells_array_name="BoundaryCells",
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    ignore_feature_zero=True,
    image_geometry_path=nx.DataPath("DataContainer"),
    include_volume_boundary=True,
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Output file path for Filter 3
output_file_path = nxtest.GetDataDirectory() + "/Output/Examples/SmallIN100_BoundaryCells.dream3d"
# Instantiate Filter
filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")