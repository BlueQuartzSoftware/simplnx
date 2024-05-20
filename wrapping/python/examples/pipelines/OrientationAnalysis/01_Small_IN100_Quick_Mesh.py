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
import_data.file_path = str(nxtest.get_data_directory() / "Output/Statistics/SmallIN100_CrystalStats.dream3d")
import_data.data_paths = None
# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                         import_data_object=import_data)

nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.CropImageGeometryFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix_path=nx.DataPath("DataContainer/Cell Feature Data"),
    feature_ids_path=nx.DataPath("DataContainer/Cell Data/FeatureIds"),
    max_voxel=[140, 140, 99],
    min_voxel=[41, 41, 0],
    remove_original_geometry=True,
    renumber_features=True,
    input_image_geometry_path=nx.DataPath("DataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
#nx_filter = nx.MoveDataFilter()
# Execute Filter with Parameters
#result = nx_filter.execute(
#    data_structure=data_structure,
#     data=[nx.DataPath("DataContainer/CellEnsembleData")],
#     new_parent=nx.DataPath("DataContainer")
# )
# nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.QuickSurfaceMeshFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    face_data_group_name=("FaceData"),
    face_feature_attribute_matrix_name=("Face Feature Data"),
    face_labels_array_name=("FaceLabels"),
    feature_ids_path=nx.DataPath("DataContainer/Cell Data/FeatureIds"),
    fix_problem_voxels=False,
    generate_triple_lines=False,
    input_grid_geometry_path=nx.DataPath("DataContainer"),
    node_types_array_name=("NodeType"),
    output_triangle_geometry_path=nx.DataPath("TriangleDataContainer"),
    vertex_data_group_name=("VertexData")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Set Output File Path
output_file_path = nxtest.get_data_directory() / "Output/SurfaceMesh/SmallIN100_Mesh.dream3d"
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
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
nxtest.cleanup_test_file(import_data.file_path)
# *****************************************************************************


print("===> Pipeline Complete")
