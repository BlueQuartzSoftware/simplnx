import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = cx.ImportDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ImportDREAM3DFilter filter")

# Filter 2
# Instantiate Filter
filter = cx.SharedFeatureFaceFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    feature_face_ids_array_name="SharedFeatureFaceId",
    feature_face_labels_array_name="FaceLabels",
    feature_num_triangles_array_name="NumTriangles",
    grain_boundary_attribute_matrix_name="SharedFeatureFace",
    randomize_features=False,
    triangle_geometry_path=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the SharedFeatureFaceFilter")

# Filter 3
# Instantiate Filter
filter = cxor.FindGBCDMetricBasedFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    chosen_limit_dists=0,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    dist_output_file=cx.DataPath("Data/Output/SurfaceMesh/7_0_small_in100_distribution_1.dat"),
    err_output_file=cx.DataPath("Data/Output/SurfaceMesh/7_0_small_in100_distribution_errors_1.dat"),
    exclude_triple_lines=False,
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    misorientation_rotation=[1.0, 1.0, 1.0, 60.0],
    node_types_array_path=cx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=False,
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=cx.DataPath("TriangleDataContainer/SharedFeatureFace/FaceLabels"),
    triangle_geometry_path=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the FindGBCDMetricBasedFilter")

# Filter 4
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Set Output File Path
output_file_path = "Data/Output/SurfaceMesh/SmallIN100_GBCD_Metric.dream3d"
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure, 
    export_file_path=output_file_path, 
    write_xdmf_file=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ExportDREAM3DFilter")
