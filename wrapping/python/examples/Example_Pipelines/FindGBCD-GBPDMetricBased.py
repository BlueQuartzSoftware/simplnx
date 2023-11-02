import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#Filter 2

result = cx.SharedFeatureFaceFilter.execute(
    data_structure=data_structure,
    face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    feature_face_ids_array_name=("FeatureFaceId"),
    feature_face_labels_array_name=("FaceLabels"),
    feature_num_triangles_array_name=("NumTriangles"),
    grain_boundary_attribute_matrix_name=("FaceFeatureData"),
    randomize_features=False,
    triangle_geometry_path=cx.DataPath("TriangleDataContainer")
)

#Filter 3

result = cxor.FindGBCDMetricBasedFilter.execute(
    data_structure=data_structure,
    chosen_limit_dists=2,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    dist_output_file=cx.DataPath("Data/Output/Examples/gbcd_distribution.dat"),
    err_output_file=cx.DataPath("Data/Output/Examples/gbcd_distribution_errors.dat"),
    exclude_triple_lines=True,
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AxisEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    misorientation_rotation=[1.0, 1.0, 1.0, 17.9],
    node_types_array_path=cx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=True,
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceFeatureData/FaceLabels"),
    triangle_geometry_path=cx.DataPath("TriangleDataContainer")
)

#Filter 4

result = cxor.FindGBCDMetricBasedFilter.execute(
    data_structure=data_structure,
    chosen_limit_dists=7,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    dist_output_file=cx.DataPath("Data/Output/Examples/gbcd_distribution.dat"),
    err_output_file=cx.DataPath("Data/Output/Examples/gbcd_distribution_errors.dat"),
    exclude_triple_lines=False,
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AxisEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    misorientation_rotation=[1.0, 1.0, 1.0, 17.9],
    node_types_array_path=cx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=False,
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceFeatureData/FaceLabels"),
    triangle_geometry_path=cx.DataPath("TriangleDataContainer")
)