import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
nx_filter = nx.CreateImageGeometry()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_data_name=("Cell Data"),
    dimensions=[100, 100, 2],
    geometry_data_path=nx.DataPath("[Image Geometry]"),
    origin=[0.0, 0.0, 0.0],
    spacing=[1.0, 1.0, 1.0]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.ReadRawBinaryFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    created_attribute_array_path=nx.DataPath("[Image Geometry]/Cell Data/Quats"),
    endian=0,
    input_file=nxtest.get_data_directory() / "OrientationAnalysis/quats.raw",
    number_of_components=4,
    scalar_type=nx.NumericType.float32,
    skip_header_bytes=0,
    tuple_dimensions=[[2.0, 100.0, 100.0]]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = cxor.ConvertOrientations()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    input_orientation_array_path=nx.DataPath("[Image Geometry]/Cell Data/Quats"),
    input_type=2,
    output_orientation_array_name="Eulers",
    output_type=0
)
nxtest.check_filter_result(nx_filter, result)


# Filter 4
# Instantiate Filter
nx_filter = nx.CreateDataArray()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    advanced_options=True,
    component_count=1,
    data_format="",
    initialization_value="1",
    numeric_type=nx.NumericType.int32,
    output_data_array=nx.DataPath("[Image Geometry]/Cell Data/Phases"),
    tuple_dimensions=[[2.0, 100.0, 100.0]]
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = cxor.ReadEnsembleInfoFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_ensemble_attribute_matrix_name=("Cell Ensemble"),
    crystal_structures_array_name=("CrystalStructures"),
    data_container_name=nx.DataPath("[Image Geometry]"),
    input_file=nxtest.get_data_directory() / "OrientationAnalysis/Ensemble.ini",
    phase_types_array_name=("PhaseTypes")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 6
# Instantiate Filter
nx_filter = cxor.GenerateIPFColorsFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_euler_angles_array_path=nx.DataPath("[Image Geometry]/Cell Data/Eulers"),
    cell_ipf_colors_array_name=("IPFColors"),
    cell_phases_array_path=nx.DataPath("[Image Geometry]/Cell Data/Phases"),
    crystal_structures_array_path=nx.DataPath("[Image Geometry]/Cell Ensemble/CrystalStructures"),
    #mask_array_path=nx.DataPath(),
    reference_dir=[0.0, 0.0, 1.0],
    use_mask=False
)
nxtest.check_filter_result(nx_filter, result)

# Filter 7
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output file path
output_file_path = nxtest.get_data_directory() / "Output/Examples/EnsembleInfoReaderExample.dream3d"
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************


print("===> Pipeline Complete")
