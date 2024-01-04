import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1

# Define ReadCSVData parameters

read_csv_data = nx.ReadCSVDataParameter()
read_csv_data.input_file_path = nxtest.GetDataDirectory() + "/ASCIIData/RectilinearGrid.csv"
read_csv_data.start_import_row = 2
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = ['x', 'y', 'z']
read_csv_data.column_data_types = [nx.DataType.float32, nx.DataType.float32, nx.DataType.float32]
read_csv_data.skipped_array_mask = [False, False, False]
read_csv_data.tuple_dims = [14]
read_csv_data.headers_line = 1
read_csv_data.header_mode = nx.ReadCSVDataParameter.HeaderMode.Custom

# Instantiate Filter
filter = nx.ReadCSVFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_data_group=nx.DataPath("Bounds"),
    read_csv_data=read_csv_data,
    # selected_data_group: DataPath = ...,
    use_existing_group=False,
)
nxtest.check_filter_result(filter, result)


# Filter 2
# Instantiate Filter
filter = nx.CreateGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_handling=1,
    cell_attribute_matrix_name="CellData",
    # dimensions: List[int] = ...,
    # edge_attribute_matrix_name: str = ...,
    # edge_list_name: DataPath = ...,
    # face_attribute_matrix_name: str = ...,
    geometry_name=nx.DataPath("RectGridGeometry"),
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
    x_bounds=nx.DataPath("Bounds/x"),
    y_bounds=nx.DataPath("Bounds/y"),
    z_bounds=nx.DataPath("Bounds/z")
)
nxtest.check_filter_result(filter, result)


# Filter 3
# Instantiate Filter
filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "/ASCIIData/ConfidenceIndex.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("RectGridGeometry/CellData/ConfidenceIndex"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)

# Filter 4
# Instantiate Filter
filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "/ASCIIData/ImageQuality.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("RectGridGeometry/CellData/ImageQuality"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)


# Filter 5
# Instantiate Filter
filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "/ASCIIData/SEM Signal.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("RectGridGeometry/CellData/SEM Signal"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)

# Filter 6
# Instantiate Filter
filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "/ASCIIData/Fit.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("RectGridGeometry/CellData/Fit"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)


# Filter 7
# Instantiate Filter
filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "/ASCIIData/EulerAngles.csv",
    n_comp=3,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("RectGridGeometry/CellData/EulerAngles"),
    scalar_type=nx.NumericType.float32
)
nxtest.check_filter_result(filter, result)

# Filter 8
# Instantiate Filter
filter = nx.ReadTextDataArrayFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="",
    delimiter_choice=0,
    input_file=nxtest.GetDataDirectory() + "/ASCIIData/Phases.csv",
    n_comp=1,
    n_skip_lines=0,
    # n_tuples: List[List[float]] = ...,
    output_data_array=nx.DataPath("RectGridGeometry/CellData/Phases"),
    scalar_type=nx.NumericType.int32
)
nxtest.check_filter_result(filter, result)


# Filter 9
# Instantiate Filter
filter = nx.ResampleRectGridToImageGeomFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    dimensions=[20, 20, 20],
    image_geom_cell_attribute_matrix="CellData",
    image_geometry_path=nx.DataPath("Image Geometry"),
    rectilinear_grid_path=nx.DataPath("RectGridGeometry"),
    selected_data_array_paths=[nx.DataPath("RectGridGeometry/CellData/ConfidenceIndex"), 
                               nx.DataPath("RectGridGeometry/CellData/EulerAngles"),
                               nx.DataPath("RectGridGeometry/CellData/Fit"),
                               nx.DataPath("RectGridGeometry/CellData/ImageQuality"),
                               nx.DataPath("RectGridGeometry/CellData/Phases"),
                               nx.DataPath("RectGridGeometry/CellData/SEM Signal")]
)
nxtest.check_filter_result(filter, result)


# Instantiate Filter
filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = nxtest.GetDataDirectory() + "/Examples/ResampleRectGridToImageGeom.dream3d"
result = filter.execute(data_structure=data_structure, 
                        export_file_path=output_file_path, 
                        write_xdmf_file=True)

nxtest.check_filter_result(filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************



print("===> Pipeline Complete")