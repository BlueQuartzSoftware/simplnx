import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/Output/Reconstruction/SmallIN100_Final.dream3d"
import_data.data_paths = None

# Instantiate Filter
filter = cx.ImportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        import_file_data=import_data)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.FindBoundaryCellsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    boundary_cells_array_name="BoundaryCells",
    feature_ids_array_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    ignore_feature_zero=True,
    image_geometry_path=cx.DataPath("DataContainer"),
    include_volume_boundary=True,
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Output file path for Filter 3
output_file_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/Output/Examples/SmallIN100_BoundaryCells.dream3d"
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")