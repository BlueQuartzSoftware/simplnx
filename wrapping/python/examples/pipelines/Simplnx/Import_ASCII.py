import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = nx.CreateGeometryFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    array_handling_index=0,
    cell_attribute_matrix_name="Cell Data",
    dimensions=[60, 80, 100],
    #edge_attribute_matrix_name: str = ...,
    #edge_list_name: DataPath = ...,
    #face_attribute_matrix_name: str = ...,
    output_geometry_path=nx.DataPath("[Image Geometry]"),
    geometry_type_index=0,
    #hexahedral_list_name: DataPath = ...,
    length_unit_index=7,
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
nxtest.check_filter_result(nx_filter, result)

# Filter 2

# Define ReadCSVData parameters
read_csv_data = nx.ReadCSVDataParameter()
read_csv_data.input_file_path = str(nxtest.get_data_directory() / "ASCIIData/EulersRotated.csv")
read_csv_data.start_import_row = 1
read_csv_data.delimiters = [',']
read_csv_data.custom_headers = ['phi1', 'Phi', 'phi2']
read_csv_data.column_data_types = [nx.DataType.float32, nx.DataType.float32, nx.DataType.float32]
read_csv_data.skipped_array_mask = [False, False, False]
read_csv_data.tuple_dims = [480_001]
read_csv_data.headers_line = 1
read_csv_data.header_mode = nx.ReadCSVDataParameter.HeaderMode.Custom

# Instantiate Filter
nx_filter = nx.ReadCSVFileFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    created_data_group_path=nx.DataPath("Imported Data"),
    read_csv_data_object=read_csv_data,
    selected_attribute_matrix_path=nx.DataPath("[Image Geometry]/Cell Data"),
    use_existing_group=True,
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    set_tuple_dimensions=False,
    component_count=1,
    data_format="",
    initialization_value_str="1",
    numeric_type_index=nx.NumericType.int32,
    output_array_path=nx.DataPath("[Image Geometry]/Cell Data/Phase")
    #tuple_dimensions=[]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.CombineAttributeArraysFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    move_values=True,
    normalize_data=False,
    input_data_array_paths=[nx.DataPath("[Image Geometry]/Cell Data/phi1"),
                               nx.DataPath("[Image Geometry]/Cell Data/Phi"),
                               nx.DataPath("[Image Geometry]/Cell Data/phi2")],
    output_data_array_name="Eulers"
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Define ensemble info parameters

ensemble_info_parameter = []
ensemble_info_parameter.append(["Hexagonal-High 6/mmm", "Primary", "Phase 1"])
ensemble_info_parameter.append(["Cubic-High m-3m", "Primary", "Phase 2"])

# Instantiate Filter
nx_filter = cxor.CreateEnsembleInfoFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_ensemble_attribute_matrix_path=nx.DataPath("[Image Geometry]/CellEnsembleData"),
    crystal_structures_array_name="CrystalStructures",
    phase_names_array_name="PhaseNames",
    phase_types_array_name="PhaseTypes",
    ensemble=ensemble_info_parameter
    )
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("[Image Geometry]/Cell Data/Eulers"),
    cell_ipf_colors_array_name="IPFColors",
    cell_phases_array_path=nx.DataPath("[Image Geometry]/Cell Data/Phase"),
    crystal_structures_array_path=nx.DataPath("[Image Geometry]/CellEnsembleData/CrystalStructures"),
    #mask_array_path: DataPath = ...,
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
    )
nxtest.check_filter_result(nx_filter, result)


# Filter 7
# Instantiate Filter
nx_filter = cxitk.ITKImageWriter()
# Output file path for Filter 7
output_file_path = nxtest.get_data_directory() / "Output/Import_ASCII/IPF.png"

# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    file_name=output_file_path,
    image_array_path=nx.DataPath("[Image Geometry]/Cell Data/IPFColors"),
    input_image_geometry_path=nx.DataPath("[Image Geometry]"),
    index_offset=0,
    plane_index=0
    )
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_dir(nxtest.get_data_directory() / "Output/Import_ASCII/")
# *****************************************************************************

print("===> Pipeline Complete")
