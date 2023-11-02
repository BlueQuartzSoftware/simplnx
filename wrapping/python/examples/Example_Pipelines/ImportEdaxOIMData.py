import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cxor.ImportH5OimDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_ensemble_attribute_matrix_name=("Cell Ensemble Data"),
    image_geometry_name=cx.DataPath("ImageGeom"),
    origin=[0.0, 0.0, 0.0],
    read_pattern_data=False,
    #selected_scan_names
    z_spacing=1.0
)

#Filter 2

result = cxor.RotateEulerRefFrameFilter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("ImageGeom/Cell Data/EulerAngles"),
    rotation_axis=[0.0, 0.0, 1.0, 90.0]
)

#Filter 3

result = cx.RotateSampleRefFrameFilter.execute(
    data_structure=data_structure,
    #created_image_geometry=cx.DataPath("DataContainer/"),
    remove_original_geometry=True,
    rotate_slice_by_slice=True,
    rotation_axis=[0.0, 1.0, 0.0, 180.0],
    #rotation_matrix: List[List[float]] = ...,
    rotation_representation=("Axis Angle"),
    selected_image_geometry=cx.DataPath("ImageGeom")
)

#Filter 4

output_file_path = "Data/Output/Examples/EdaxOIMData.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")