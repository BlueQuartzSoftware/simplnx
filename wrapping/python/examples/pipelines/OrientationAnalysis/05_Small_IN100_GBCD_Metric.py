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
import_data.file_path = str(nxtest.get_data_directory() / "Output/SurfaceMesh/SmallIN100_MeshStats.dream3d")
import_data.data_paths =[
            nx.DataPath("DataContainer"),
            nx.DataPath("DataContainer/CellData"),
            nx.DataPath("DataContainer/Cell Data/Image Quality"),
            nx.DataPath("DataContainer/Cell Data/Confidence Index"),
            nx.DataPath("DataContainer/Cell Data/SEM Signal"),
            nx.DataPath("DataContainer/Cell Data/Fit"),
            nx.DataPath("DataContainer/Cell Data/EulerAngles"),
            nx.DataPath("DataContainer/Cell Data/Phases"),
            nx.DataPath("DataContainer/Cell Data/Mask"),
            nx.DataPath("DataContainer/Cell Data/Quats"),
            nx.DataPath("DataContainer/Cell Data/FeatureIds"),
            nx.DataPath("DataContainer/Cell Data/ParentIds"),
            nx.DataPath("DataContainer/Cell Data/IPFColors"),
            nx.DataPath("DataContainer/Cell Data/GBManhattanDistances"),
            nx.DataPath("DataContainer/Cell Data/TJManhattanDistances"),
            nx.DataPath("DataContainer/Cell Data/QPManhattanDistances"),
            nx.DataPath("DataContainer/Cell Data/FeatureReferenceMisorientations"),
            nx.DataPath("DataContainer/Cell Data/KernelAverageMisorientations"),
            nx.DataPath("DataContainer/Cell Feature Data"),
            nx.DataPath("DataContainer/Cell Ensemble Data"),
            nx.DataPath("DataContainer/Cell Ensemble Data/CrystalStructures"),
            nx.DataPath("DataContainer/Cell Ensemble Data/LatticeConstants"),
            nx.DataPath("DataContainer/NewGrain Data"),
            nx.DataPath("DataContainer/NewGrain Data/Active"),
            nx.DataPath("DataContainer/Cell Feature Data/Active"),
            nx.DataPath("DataContainer/Cell Feature Data/AspectRatios"),
            nx.DataPath("DataContainer/Cell Feature Data/AvgEulerAngles"),
            nx.DataPath("DataContainer/Cell Feature Data/AvgQuats"),
            nx.DataPath("DataContainer/Cell Feature Data/AxisEulerAngles"),
            nx.DataPath("DataContainer/Cell Feature Data/AxisLengths"),
            nx.DataPath("DataContainer/Cell Feature Data/Centroids"),
            nx.DataPath("DataContainer/Cell Feature Data/EquivalentDiameters"),
            nx.DataPath("DataContainer/Cell Feature Data/FeatureAvgMisorientations"),
            nx.DataPath("DataContainer/Cell Feature Data/Neighborhoods"),
            nx.DataPath("DataContainer/Cell Feature Data/NumElements"),
            nx.DataPath("DataContainer/Cell Feature Data/NumNeighbors"),
            #nx.DataPath("DataContainer/Cell Feature Data/NumNeighbors2"),
            nx.DataPath("DataContainer/Cell Feature Data/Omega3s"),
            nx.DataPath("DataContainer/Cell Feature Data/ParentIds"),
            nx.DataPath("DataContainer/Cell Feature Data/Phases"),
            nx.DataPath("DataContainer/Cell Feature Data/Poles"),
            nx.DataPath("DataContainer/Cell Feature Data/Schmids"),
            nx.DataPath("DataContainer/Cell Feature Data/Shape Volumes"),
            nx.DataPath("DataContainer/Cell Feature Data/SlipSystems"),
            nx.DataPath("DataContainer/Cell Feature Data/SurfaceAreaVolumeRatio"),
            nx.DataPath("TriangleDataContainer"),
            nx.DataPath("TriangleDataContainer/SharedTriList"),
            nx.DataPath("TriangleDataContainer/SharedVertexList"),
            nx.DataPath("TriangleDataContainer/FaceData"),
            nx.DataPath("TriangleDataContainer/VertexData"),
            nx.DataPath("TriangleDataContainer/VertexData/NodeType"),
            nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
            nx.DataPath("TriangleDataContainer/Edge List"),
            nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
            nx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
            nx.DataPath("TriangleDataContainer/FaceData/FaceDihedralAngles")
          ]

# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        import_data_object=import_data)

nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = nx.SharedFeatureFaceFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    feature_face_ids_array_name="SharedFeatureFaceId",
    feature_face_labels_array_name="FaceLabels",
    feature_num_triangles_array_name="NumTriangles",
    grain_boundary_attribute_matrix_name="SharedFeatureFace",
    randomize_features=False,
    input_triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = cxor.ComputeGBCDMetricBasedFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    chosen_limit_dists_index=0,
    crystal_structures_array_path=nx.DataPath("DataContainer/Cell Ensemble Data/CrystalStructures"),
    dist_output_file="Data/Output/SurfaceMesh/7_0_small_in100_distribution_1.dat",
    err_output_file="Data/Output/SurfaceMesh/7_0_small_in100_distribution_errors_1.dat",
    exclude_triple_lines=False,
    feature_euler_angles_array_path=nx.DataPath("DataContainer/Cell Feature Data/AvgEulerAngles"),
    feature_phases_array_path=nx.DataPath("DataContainer/Cell Feature Data/Phases"),
    misorientation_rotation=[1.0, 1.0, 1.0, 60.0],
    node_types_array_path=nx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=False,
    surface_mesh_face_areas_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=nx.DataPath("TriangleDataContainer/SharedFeatureFace/FaceLabels"),
    input_triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.get_data_directory() / "Output/SurfaceMesh/SmallIN100_GBCD_Metric.dream3d"
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
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************

print("===> Pipeline Complete")
