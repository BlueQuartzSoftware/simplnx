import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1

# Define ReadCSVData parameters

read_csv_data = cx.ReadCSVDataParameter()
read_csv_data.input_file_path = cxtest.GetDataDirectory() + "Data/ASCIIData/RectilinearGrid.csv"
read_csv_data.start_import_row = 2
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = ['x', 'y', 'z']
read_csv_data.column_data_types = [cx.DataType.float32, cx.DataType.float32, cx.DataType.float32]
read_csv_data.skipped_array_mask = [False, False, False]
read_csv_data.tuple_dims = [14]
read_csv_data.headers_line = 1
read_csv_data.header_mode = cx.ReadCSVDataParameter.HeaderMode.Custom

# Instantiate Filter
filter = cx.ReadCSVFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_data_group=cx.DataPath("Bounds"),
    read_csv_data=read_csv_data,
    # selected_data_group: DataPath = ...,
    use_existing_group=False,
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 2
# Instantiate Filter
filter = cx.CreateGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_handling=1,
    cell_attribute_matrix_name="CellData",
    # dimensions: List[int] = ...,
    # edge_attribute_matrix_name: str = ...,
    # edge_list_name: DataPath = ...,
    # face_attribute_matrix_name: str = ...,
    geometry_name=cx.DataPath("RectGridGeometry"),
    geometry_type=1,
    # hexahedral_list_name: DataPath = ...,
    length_unit_type=7,
    # origin: List[float] = ...,
    # quadrilateral_list_name: DataPath = ...,
    # spacing: List[float] = ...,
    # tetrahedral_list_name: DataPath = ...,
    # triangle_list_name: DataPath = ...,
    # vertex_attribute_matrix_name: str = ...,
    # vertex_list_name: DataPath = ...,
    warnings_as_errors=False,
    x_bounds=cx.DataPath("Bounds/x"),
    y_bounds=cx.DataPath("Bounds/y"),
    z_bounds=cx.DataPath("Bounds/z")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 3
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=cxtest.GetDataDirectory() + "Data/ASCIIData/ConfidenceIndex.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("RectGridGeometry/CellData/ConfidenceIndex"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=cxtest.GetDataDirectory() + "Data/ASCIIData/ImageQuality.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("RectGridGeometry/CellData/ImageQuality"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 5
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=cxtest.GetDataDirectory() + "Data/ASCIIData/SEM Signal.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("RectGridGeometry/CellData/SEM Signal"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=cxtest.GetDataDirectory() + "Data/ASCIIData/Fit.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("RectGridGeometry/CellData/Fit"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 7
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=cxtest.GetDataDirectory() + "Data/ASCIIData/EulerAngles.csv",
    n_comp=3,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("RectGridGeometry/CellData/EulerAngles"),
    scalar_type=cx.NumericType.float32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=cxtest.GetDataDirectory() + "Data/ASCIIData/Phases.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("RectGridGeometry/CellData/Phases"),
    scalar_type=cx.NumericType.int32
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 9
# Instantiate Filter
filter = cx.ResampleRectGridToImageGeomFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    dimensions=[20, 20, 20],
    image_geom_cell_attribute_matrix="CellData",
    image_geometry_path=cx.DataPath("Image Geometry"),
    rectilinear_grid_path=cx.DataPath("RectGridGeometry"),
    selected_data_array_paths=[cx.DataPath("RectGridGeometry/CellData/ConfidenceIndex"), 
                               cx.DataPath("RectGridGeometry/CellData/EulerAngles"),
                               cx.DataPath("RectGridGeometry/CellData/Fit"),
                               cx.DataPath("RectGridGeometry/CellData/ImageQuality"),
                               cx.DataPath("RectGridGeometry/CellData/Phases"),
                               cx.DataPath("RectGridGeometry/CellData/SEM Signal")]
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = cxtest.GetDataDirectory() + "Data/Examples/ResampleRectGridToImageGeom.dream3d"
result = filter.execute(data_structure=data_structure, 
                        export_file_path=output_file_path, 
                        write_xdmf_file=True)

if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

print("===> Pipeline Complete")