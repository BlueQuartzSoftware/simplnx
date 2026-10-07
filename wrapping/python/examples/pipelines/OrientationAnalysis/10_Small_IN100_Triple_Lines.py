import simplnx as nx
import simplnx_test_dirs as nxtest

# Run after 02_Small_IN100_Smooth_Mesh.py and before 03_Small_IN100_Mesh_Statistics.py.
# Keep the smoothed input file until the mesh statistics pipeline has used it.
data_structure = nx.DataStructure()

# Read the smoothed surface mesh.
file_path = str(nxtest.get_data_directory() / "Output/SurfaceMesh/SmallIN100_Smoothed.dream3d")
import_data = nx.Dream3dImportParameter.ImportData(file_path=file_path)
nx_filter = nx.ReadDREAM3DFilter()
result = nx_filter.execute(data_structure=data_structure, import_data_object=import_data)
nxtest.check_filter_result(nx_filter, result)

# Extract interior triple lines and copy Node Types from the surface mesh.
nx_filter = nx.ExtractTripleLinesFilter()
result = nx_filter.execute(
    data_structure=data_structure,
    copy_node_types=True,
    edge_data_group_name="Edge Data",
    face_labels_array_path=nx.DataPath("TriangleDataContainer/Face Data/FaceLabels"),
    include_exterior_triple_lines=False,
    input_triangle_geometry_path=nx.DataPath("TriangleDataContainer"),
    node_types_array_name="NodeTypes",
    node_types_array_path=nx.DataPath("TriangleDataContainer/Vertex Data/NodeType"),
    num_features_array_name="NumFeatures",
    output_triple_line_geometry_path=nx.DataPath("Triple Lines"),
    vertex_data_group_name="Vertex Data",
)
nxtest.check_filter_result(nx_filter, result)

# Save the surface mesh and extracted triple lines.
nx_filter = nx.WriteDREAM3DFilter()
output_file_path = nxtest.get_data_directory() / "Output/SurfaceMesh/SmallIN100_TripleLines.dream3d"
result = nx_filter.execute(
    data_structure=data_structure,
    compression_level=5,
    export_file_path=output_file_path,
    use_compression=True,
    write_xdmf_file=True,
)
nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")
