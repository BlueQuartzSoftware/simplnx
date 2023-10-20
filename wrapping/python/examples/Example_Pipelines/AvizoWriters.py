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

result = cx.ImportDREAM3DFilter.execute(
    data_structure=data_structure,
    import_file_data=import_data
)
#Filter 2
result = cx.AvizoUniformCoordinateWriterFilter.execute(
    data_structure=data_structure,
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    output_file=cx.DataPath("Data/Output/Examples/SmallIN100_AvizoUniform.am"),
    units="meters",
    write_binary_file=False,
)
#Filter 3
result = cx.AvizoUniformCoordinateWriterFilter.execute(
    data_structure=data_structure,
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    output_file=cx.DataPath("Data/Output/Examples/SmallIN100_AvizoRectilinear.am"),
    units="meters",
    write_binary_file=False,
)