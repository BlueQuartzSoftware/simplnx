import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#Filter 2

result = cx.FindBoundaryCellsFilter.execute(data_structure=data_structure,
    boundary_cells_array_name="BoundaryCells",
    feature_ids_array_path=cx.DataPath(["DataContainer/CellData/FeatureIds"]),
    ignore_feature_zero=True,
    image_geometry_path=cx.DataPath(["DataContainer"]),
    include_volume_boundary=True
)

#Filter 3

output_file_path = "Data/Output/Examples/SmallIN100_BoundaryCells.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")
