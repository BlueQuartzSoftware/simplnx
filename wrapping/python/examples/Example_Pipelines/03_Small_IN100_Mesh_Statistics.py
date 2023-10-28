import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/SurfaceMesh/SmallIN100_Smoothed.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#FIlter 2

result = cx.CalculateTriangleAreasFilter.execute(
    data_structure=data_structure,
    triangle_areas_array_path=("FaceAreas"),
    triangle_geometry_data_path=cx.DataPath("TriangleDataContainer"),
)
#Filter 3

result = cx.TriangleNormalFilter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_normals_array_path=("FaceNormals"),
    tri_geometry_data_path=cx.DataPath("TriangleDataContainer"),
)
#Filter 4

result = cx.TriangleDihedralAngleFilter.execute(
    data_structure=data_structure,
    surface_mesh_triangle_dihedral_angles_array_name=("FaceDihedralAngles"),
    tri_geometry_data_path=cx.DataPath("TriangleDataContainer"),
)
#Filter 5

result = cxor.GenerateFaceIPFColoringFilter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    surface_mesh_face_ipf_colors_array_name=("FaceIPFColors"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
)
#Filter 6

result = cxor.GenerateFaceMisorientationColoringFilter.execute(
    data_structure=data_structure,
    avg_quats_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_misorientation_colors_array_name=("FaceMisorientationColors"),
)
#Filter 7

result = cx.SharedFeatureFaceFilter.execute(
    data_structure=data_structure,
    face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabel"),
    feature_face_ids_array_name=("SharedFeatureFaceId"),
    feature_face_labels_array_name=("FaceLabels"),
    feature_num_triangles_array_name=("NumTriangles"),
    grain_boundary_attribute_matrix_name=("SharedFeatureFace"),
    randomize_features=False,
    triangle_geometry_path=cx.DataPath("TriangleDataContainer"),
)
#Filter 8

output_file_path = "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")