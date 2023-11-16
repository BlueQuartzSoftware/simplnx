import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cx.CreateImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_data_name="CellData",
    dimensions=[60, 40, 100],
    geometry_data_path=cx.DataPath("[Image Geometry]"),
    origin=[0.0, 0.0, 0.0],
    spacing=[0.25, 0.25, 1.0]
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 2
# Define ReadCSVData parameters
read_csv_data = cx.ReadCSVData()
read_csv_data.input_file_path = "C:/Users/alejo/Downloads/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/DREAM3DNX-7.0.0-RC-7-UDRI-20231027.2-windows-AMD64/Data/ASCIIData/EulersRotated.csv"
read_csv_data.start_import_row = 2
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = ['phi1', 'Phi', 'phi2']
read_csv_data.column_data_types = [cx.DataType.float32, cx.DataType.float32, cx.DataType.float32]
read_csv_data.skipped_array_mask = [False, False, False]
read_csv_data.tuple_dims = [480_001]
read_csv_data.headers_line = 1
read_csv_data.header_mode = cx.ReadCSVData.HeaderMode.Custom

# Instantiate Filter
filter = cx.ReadCSVFileFilter()
# Execute Filter with Parameters
# This will store the imported arrays into a newly generated DataGroup
result = filter.execute(data_structure=data_structure,
                        # This will store the imported arrays into a newly generated DataGroup
                        created_data_group=cx.DataPath(["Imported Data"]),
                        # We are not using this parameter but it still needs a value
                        selected_data_group=cx.DataPath("[Image Geometry]/CellData"),
                        # Use an existing DataGroup or AttributeMatrix. If an AttributemMatrix is used, the total number of tuples must match
                        use_existing_group=True,
                        # The ReadCSVData object with all member variables set.
                        read_csv_data=read_csv_data)
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.CombineAttributeArraysFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    move_values=True,
    normalize_data=False,
    selected_data_array_paths=[cx.DataPath(["[Image Geometry]/CellData/phil"]),
                               cx.DataPath(["[Image Geometry]/CellData/Phi"]),
                               cx.DataPath(["[Image Geometry]/CellData/phi2"])],
    stacked_data_array_name="Eulers"
)
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")

# Filter 4
# Define ensemble info parameters
ensemble_info_parameter = []
ensemble_info_parameter.append(["Hexagonal-High 6/mmm", "Primary", "Phase 1"])
ensemble_info_parameter.append(["Cubic-High m-3m", "Primary", "Phase 2"])

# Instantiate Filter
filter = cxor.CreateEnsembleInfoFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_ensemble_attribute_matrix_name=cx.DataPath(["CellEnsembleData"]),
    crystal_structures_array_name="CrystalStructures",
    phase_names_array_name="PhaseNames",
    phase_types_array_name="PhaseTypes",
    ensemble=ensemble_info_parameter
    )
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")


# Filter 5
# Instantiate Filter
filter = cx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format='Unknown',
    initialization_value='1',
    numeric_type=4,
    output_data_array=cx.DataPath("Phases"),
    #tuple_dimensions: List[List[float]] = ...
    )
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=cx.DataPath("[Image Geometry]/CellData/Phase"),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=cx.DataPath("[Image Geometry]/CellData/Phase"),
    crystal_structures_array_path=cx.DataPath("[Image Geometry]/CellEnsembleData/CrystalStructures"),
    #mask_array_path: DataPath = ...,
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
    )
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")


# Filter 7
# Instantiate Filter
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name=cx.DataPath("Data/Output/Import_ASCII_IPF.png"),
    image_array_path=cx.DataPath("[Image Geometry]/CellData/IPFColors"),
    image_geom_path=cx.DataPath("[Image Geometry]"),
    index_offset=0,
    plane=0
    )
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
    quit()
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Define output file path
output_file_path = "Data/Output/Small_IN100/EnsembleData.dream3d"
# Execute ExportDREAM3DFilter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
if len(result.errors) != 0:
    print(f'Errors: {result.errors}')
elif len(result.warnings) != 0:
    print(f'Warnings: {result.warnings}')
else:
    print("No errors running the filter")