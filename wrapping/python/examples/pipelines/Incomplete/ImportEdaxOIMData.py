import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadH5OimDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("Cell Ensemble Data"),
    image_geometry_name=cx.DataPath("ImageGeom"),
    origin=[0.0, 0.0, 0.0],
    read_pattern_data=False,
    z_spacing=1.0
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = cxor.RotateEulerRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    euler_angles_array_path=cx.DataPath("ImageGeom/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = cx.RotateSampleRefFrameFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    remove_original_geometry=True,
    rotate_slice_by_slice=True,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    rotation_representation=("Axis Angle"),
    selected_image_geometry=cx.DataPath("ImageGeom")
)
nxtest.check_filter_result(filter, result)

# Filter 4
# Define output file path
output_file_path = nxtest.GetDataDirectory() + "/Output/EdaxOIMData/EdaxOIMData.dream3d"
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute WriteDREAM3DFilter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")