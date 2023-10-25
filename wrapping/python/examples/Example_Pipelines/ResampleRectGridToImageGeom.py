import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

result = cx.ImportCSVDataFilter.execute(
    data_structure=data_structure,
    created_data_group=cx.DataPath("Bounds"),
    #selected_data_group: DataPath = ...,
    tuple_dimensions=[14.0],
    use_existing_group=False,
    #wizard_data: CSVWizardData = ...
)
#Filter 2

result = cx.CreateGeometryFilter.execute(
    data_structure=data_structure,
    array_handling=1,
    cell_attribute_matrix_name="CellData",
    #dimensions: List[int] = ...,
    #edge_attribute_matrix_name: str = ...,
    #edge_list_name: DataPath = ...,
    #face_attribute_matrix_name: str = ...,
    geometry_name=cx.DataPath("RectGridGeometry"),
    geometry_type=1,
    #hexahedral_list_name: DataPath = ...,
    length_unit_type=7,
    #origin: List[float] = ...,
    #quadrilateral_list_name: DataPath = ...,
    #spacing: List[float] = ...,
    #tetrahedral_list_name: DataPath = ...,
    #triangle_list_name: DataPath = ...,
    #vertex_attribute_matrix_name: str = ...,
    #vertex_list_name: DataPath = ...,
    warnings_as_errors=False,
    x_bounds=cx.DataPath("Bounds/x"),
    y_bounds=cx.DataPath("Bounds/y"),
    z_bounds=cx.DataPath("Bounds/z"),
)
#Filter 3

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file="Data/ASCIIData/ConfidenceIndex.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("ConfidenceIndex"),
    scalar_type=8,
)
#Filter 4

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file="Data/ASCIIData/ImageQuality.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("ImageQuality"),
    scalar_type=8,
)
#Filter 5

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file="Data/ASCIIData/SEM Signal.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("SEM Signal"),
    scalar_type=8,
)
#Filter 6

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file="Data/ASCIIData/Fit.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("Fit"),
    scalar_type=8,
)
#Filter 7

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file="Data/ASCIIData/EulerAngles.csv",
    n_comp=3,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("EulerAngles"),
    scalar_type=8,
)
#Filter 8

result = cx.ImportTextFilter.execute(
    data_structure=data_structure,
    advanced_options=False,
    data_format="Unknown",
    delimiter_choice=0,
    input_file="Data/ASCIIData/Phases.csv",
    n_comp=1,
    n_skip_lines=0,
    #n_tuples: List[List[float]] = ...,
    output_data_array=cx.DataPath("Phases"),
    scalar_type=4,
)
#Filter 9

result = cx.ResampleRectGridToImageGeomFilter.execute(
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
#Filter 10

output_file_path = "Data/Examples/ResampleRectGridToImageGeom.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")