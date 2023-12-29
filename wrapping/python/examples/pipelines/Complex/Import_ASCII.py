import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = nx.CreateGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    array_handling=0,
    cell_attribute_matrix_name="Cell Data",
    dimensions=[60, 80, 100],
    #edge_attribute_matrix_name: str = ...,
    #edge_list_name: DataPath = ...,
    #face_attribute_matrix_name: str = ...,
    geometry_name=nx.DataPath("[Image Geometry]"),
    geometry_type=0,
    #hexahedral_list_name: DataPath = ...,
    length_unit_type=7,
    origin=[0, 0, 0],
    #quadrilateral_list_name: DataPath = ...,
    spacing=[1, 1, 1],
    #tetrahedral_list_name: DataPath = ...,
    #triangle_list_name: DataPath = ...,
    #vertex_attribute_matrix_name: str = ...,
    #vertex_list_name: DataPath = ...,
    warnings_as_errors=False,
    #x_bounds: DataPath = ...,
    #y_bounds: DataPath = ...,
    #z_bounds: DataPath = ...
)
nxtest.check_filter_result(filter, result)

# Filter 2

# Define ReadCSVData parameters
read_csv_data = nx.ReadCSVDataParameter()
read_csv_data.input_file_path = nxtest.GetDataDirectory() + "/ASCIIData/EulersRotated.csv"
read_csv_data.start_import_row = 1
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = ['phi1', 'Phi', 'phi2']
read_csv_data.column_data_types = [nx.DataType.float32, nx.DataType.float32, nx.DataType.float32]
read_csv_data.skipped_array_mask = [False, False, False]
read_csv_data.tuple_dims = [480_001]
read_csv_data.headers_line = 1
read_csv_data.header_mode = nx.ReadCSVDataParameter.HeaderMode.Custom

# Instantiate Filter
filter = nx.ReadCSVFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    created_data_group=nx.DataPath("Imported Data"),
    read_csv_data=read_csv_data,
    selected_data_group=nx.DataPath("[Image Geometry]/Cell Data"),
    use_existing_group=True,
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    advanced_options=False,
    component_count=1,
    data_format="",
    initialization_value="1",
    numeric_type=nx.NumericType.int32,
    output_data_array=nx.DataPath("[Image Geometry]/Cell Data/Phase")
    #tuple_dimensions=[]
)
nxtest.check_filter_result(filter, result)

# Filter 4
# Instantiate Filter
filter = nx.CombineAttributeArraysFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    move_values=True,
    normalize_data=False,
    selected_data_array_paths=[nx.DataPath("[Image Geometry]/Cell Data/phi1"),
                               nx.DataPath("[Image Geometry]/Cell Data/Phi"),
                               nx.DataPath("[Image Geometry]/Cell Data/phi2")],
    stacked_data_array_name="Eulers"
)
nxtest.check_filter_result(filter, result)

# Filter 5
# Define ensemble info parameters

ensemble_info_parameter = []
ensemble_info_parameter.append(["Hexagonal-High 6/mmm", "Primary", "Phase 1"])
ensemble_info_parameter.append(["Cubic-High m-3m", "Primary", "Phase 2"])

# Instantiate Filter
filter = cxor.CreateEnsembleInfoFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_ensemble_attribute_matrix_name=nx.DataPath("[Image Geometry]/CellEnsembleData"),
    crystal_structures_array_name="CrystalStructures",
    phase_names_array_name="PhaseNames",
    phase_types_array_name="PhaseTypes",
    ensemble=ensemble_info_parameter
    )
nxtest.check_filter_result(filter, result)

# Filter 6
# Instantiate Filter
filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("[Image Geometry]/Cell Data/Eulers"),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=nx.DataPath("[Image Geometry]/Cell Data/Phase"),
    crystal_structures_array_path=nx.DataPath("[Image Geometry]/CellEnsembleData/CrystalStructures"),
    #mask_array_path: DataPath = ...,
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
    )
nxtest.check_filter_result(filter, result)


# Filter 7
# Instantiate Filter
filter = cxitk.ITKImageWriter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    file_name=nxtest.GetDataDirectory() + "/Output/Import_ASCII_IPF.png",
    image_array_path=nx.DataPath("[Image Geometry]/Cell Data/IPFColors"),
    image_geom_path=nx.DataPath("[Image Geometry]"),
    index_offset=0,
    plane=0
    )
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")