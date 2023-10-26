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

result = cx.VtkRectilinearGridWriterFilter.execute(
    data_structure=data_structure,
    image_geometry_path=cx.DataPath("DataContainer"),
    output_file=cx.DataPath("Data/Output/Examples/SmallIN100_Final.vtk"),
    write_binary_file=True,
    selected_data_array_paths=[
 cx.DataPath("DataContainer/CellData/Confidence Index"), 
 cx.DataPath("DataContainer/CellData/EulerAngles"),
 cx.DataPath("DataContainer/CellData/FeatureIds"),
 cx.DataPath("DataContainer/CellData/Fit"),
 cx.DataPath("DataContainer/CellData/IPFColors"),
 cx.DataPath("DataContainer/CellData/Image Quality"),
 cx.DataPath("DataContainer/CellData/Mask"),
 cx.DataPath("DataContainer/CellData/ParentIds"),
 cx.DataPath("DataContainer/CellData/Phases"),
 cx.DataPath("DataContainer/CellData/Quats"),
 cx.DataPath("DataContainer/CellData/SEM Signal"),
 cx.DataPath("DataContainer/CellData/X Position"),
 cx.DataPath("DataContainer/CellData/Y Position")]
)