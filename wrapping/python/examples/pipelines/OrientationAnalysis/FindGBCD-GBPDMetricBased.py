import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Import Data
import_data = nx.Dream3dImportParameter.ImportData()
import_data.file_path = nxtest.GetDataDirectory() + "/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        import_file_data=import_data)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = nx.SharedFeatureFaceFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    feature_face_ids_array_name=("FeatureFaceId"),
    feature_face_labels_array_name=("FaceLabels"),
    feature_num_triangles_array_name=("NumTriangles"),
    grain_boundary_attribute_matrix_name=("FaceFeatureData"),
    randomize_features=False,
    triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(filter, result)


# Filter 3
# Instantiate Filter
filter = cxor.FindGBCDMetricBasedFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    chosen_limit_dists=2,
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    dist_output_file=nxtest.GetDataDirectory() + "/Output/Examples/gbcd_distribution.dat",
    err_output_file=nxtest.GetDataDirectory() + "/Output/Examples/gbcd_distribution_errors.dat",
    exclude_triple_lines=True,
    feature_euler_angles_array_path=nx.DataPath("DataContainer/CellFeatureData/AxisEulerAngles"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    misorientation_rotation=[1.0, 1.0, 1.0, 17.9],
    node_types_array_path=nx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=True,
    surface_mesh_face_areas_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceFeatureData/FaceLabels"),
    triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(filter, result)


# Filter 4
# Instantiate Filter
filter = cxor.FindGBPDMetricBasedFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    dist_output_file=nxtest.GetDataDirectory() + "/Output/Examples/gbpd_distribution.dat",
    err_output_file=nxtest.GetDataDirectory() + "/Output/Examples/gbpd_distribution_errors.dat",
    exclude_triple_lines=False,
    feature_euler_angles_array_path=nx.DataPath("DataContainer/CellFeatureData/AxisEulerAngles"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    limit_dist=7.0,
    node_types_array_path=nx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=False,
    surface_mesh_face_areas_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceFeatureData/FaceLabels"),
    triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(filter, result)

print("===> Pipeline Complete")