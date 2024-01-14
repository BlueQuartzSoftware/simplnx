import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = cxor.ReadH5OimDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("Cell Ensemble Data"),
    image_geometry_name=cx.DataPath("ImageGeom"),
    origin=[0.0, 0.0, 0.0],
    read_pattern_data=False,
    #selected_scan_names: ValueType = ...,
    z_spacing=1.0
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Define output file path
output_file_path = nxtest.GetDataDirectory() + "/Output/H5EspritData/H5EspritData.dream3d"
# Instantiate Filter
nx_filter = cx.WriteDREAM3DFilter()
# Execute WriteDREAM3DFilter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")
