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
nx_filter = nx.FindBoundaryCellsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    boundary_cells_array_name="BoundaryCells",
    feature_ids_array_path=nx.DataPath("DataContainer/CellData/FeatureIds"),
    ignore_feature_zero=True,
    image_geometry_path=nx.DataPath("DataContainer"),
    include_volume_boundary=True,
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Output file path for Filter 3
output_file_path = nxtest.get_data_directory() / "Output/FindBoundaryCells/SmallIN100_BoundaryCells.dream3d"
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************

print("===> Pipeline Complete")
