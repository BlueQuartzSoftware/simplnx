import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = nxtest.GetDataDirectory() + "/Output/SurfaceMesh/SmallIN100_Mesh.dream3d"
import_data.data_paths = None
# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure, import_file_data=import_data)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.LaplacianSmoothingFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    iteration_steps=100,
    lambda_value=0.25,
    mu_factor=0.2,
    quad_point_lambda=0.15000001,
    surface_mesh_node_type_array_path=nx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    surface_point_lambda=0.0,
    surface_quad_point_lambda=0.0,
    surface_triple_line_lambda=0.0,
    triangle_geometry_data_path=nx.DataPath("TriangleDataContainer"),
    triple_line_lambda=0.2,
    use_taubin_smoothing=True
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.GetDataDirectory() + "/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d"
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(import_data.file_path)
# *****************************************************************************



print("===> Pipeline Complete")
