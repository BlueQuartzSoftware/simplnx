import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d"
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
filter = cx.CalculateTriangleAreasFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    triangle_areas_array_path="FaceAreas",
    triangle_geometry_data_path=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
filter = cx.TriangleNormalFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_normals_array_path="FaceNormals",
    tri_geometry_data_path=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.TriangleDihedralAngleFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_dihedral_angles_array_name="FaceDihedralAngles",
    tri_geometry_data_path=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Instantiate Filter
filter = cxor.GenerateFaceIPFColoringFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    surface_mesh_face_ipf_colors_array_name="FaceIPFColors",
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 6
# Instantiate Filter
filter = cxor.GenerateFaceMisorientationColoringFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_misorientation_colors_array_name="FaceMisorientationColors"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.SharedFeatureFaceFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabel"),
    feature_face_ids_array_name="SharedFeatureFaceId",
    feature_face_labels_array_name="FaceLabels",
    feature_num_triangles_array_name="NumTriangles",
    grain_boundary_attribute_matrix_name="SharedFeatureFace",
    randomize_features=False,
    triangle_geometry_path=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 8
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
# Check result for errors or warnings
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ExportDREAM3DFilter")