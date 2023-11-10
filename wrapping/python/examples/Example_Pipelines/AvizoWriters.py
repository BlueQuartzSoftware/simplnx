import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.ImportDREAM3DFilter()
# Set import parameters
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.AvizoUniformCoordinateWriterFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    output_file=cx.DataPath("Data/Output/Examples/SmallIN100_AvizoUniform.am"),
    units="meters",
    write_binary_file=False)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.AvizoUniformCoordinateWriterFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    geometry_path=cx.DataPath("DataContainer"),
    output_file=cx.DataPath("Data/Output/Examples/SmallIN100_AvizoRectilinear.am"),
    units="meters",
    write_binary_file=False)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")