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
import_data.file_path = nxtest.GetDataDirectory() + "/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
import_data.data_paths =[
            nx.DataPath("DataContainer"),
            nx.DataPath("DataContainer/CellData"),
            nx.DataPath("DataContainer/CellData/Image Quality"),
            nx.DataPath("DataContainer/CellData/Confidence Index"),
            nx.DataPath("DataContainer/CellData/SEM Signal"),
            nx.DataPath("DataContainer/CellData/Fit"),
            nx.DataPath("DataContainer/CellData/EulerAngles"),
            nx.DataPath("DataContainer/CellData/Phases"),
            nx.DataPath("DataContainer/CellData/Mask"),
            nx.DataPath("DataContainer/CellData/Quats"),
            nx.DataPath("DataContainer/CellData/FeatureIds"),
            nx.DataPath("DataContainer/CellData/ParentIds"),
            nx.DataPath("DataContainer/CellData/IPFColors"),
            nx.DataPath("DataContainer/CellData/GBManhattanDistances"),
            nx.DataPath("DataContainer/CellData/TJManhattanDistances"),
            nx.DataPath("DataContainer/CellData/QPManhattanDistances"),
            nx.DataPath("DataContainer/CellData/FeatureReferenceMisorientations"),
            nx.DataPath("DataContainer/CellData/KernelAverageMisorientations"),
            nx.DataPath("DataContainer/CellFeatureData"),
            nx.DataPath("DataContainer/CellEnsembleData"),
            nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
            nx.DataPath("DataContainer/CellEnsembleData/LatticeConstants"),
            nx.DataPath("DataContainer/NewGrain Data"),
            nx.DataPath("DataContainer/NewGrain Data/Active"),
            nx.DataPath("DataContainer/CellFeatureData/Active"),
            nx.DataPath("DataContainer/CellFeatureData/AspectRatios"),
            nx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
            nx.DataPath("DataContainer/CellFeatureData/AvgQuats"),
            nx.DataPath("DataContainer/CellFeatureData/AxisEulerAngles"),
            nx.DataPath("DataContainer/CellFeatureData/AxisLengths"),
            nx.DataPath("DataContainer/CellFeatureData/Centroids"),
            nx.DataPath("DataContainer/CellFeatureData/EquivalentDiameters"),
            nx.DataPath("DataContainer/CellFeatureData/FeatureAvgMisorientations"),
            nx.DataPath("DataContainer/CellFeatureData/Neighborhoods"),
            nx.DataPath("DataContainer/CellFeatureData/NumElements"),
            nx.DataPath("DataContainer/CellFeatureData/NumNeighbors"),
            #nx.DataPath("DataContainer/CellFeatureData/NumNeighbors2"),
            nx.DataPath("DataContainer/CellFeatureData/Omega3s"),
            nx.DataPath("DataContainer/CellFeatureData/ParentIds"),
            nx.DataPath("DataContainer/CellFeatureData/Phases"),
            nx.DataPath("DataContainer/CellFeatureData/Poles"),
            nx.DataPath("DataContainer/CellFeatureData/Schmids"),
            nx.DataPath("DataContainer/CellFeatureData/Shape Volumes"),
            nx.DataPath("DataContainer/CellFeatureData/SlipSystems"),
            nx.DataPath("DataContainer/CellFeatureData/SurfaceAreaVolumeRatio"),
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
                        import_file_data=import_data)

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
    triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 3
# Instantiate Filter
nx_filter = cxor.FindGBCDMetricBasedFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    chosen_limit_dists=0,
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    dist_output_file="Data/Output/SurfaceMesh/7_0_small_in100_distribution_1.dat",
    err_output_file="Data/Output/SurfaceMesh/7_0_small_in100_distribution_errors_1.dat",
    exclude_triple_lines=False,
    feature_euler_angles_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    misorientation_rotation=[1.0, 1.0, 1.0, 60.0],
    node_types_array_path=nx.DataPath("TriangleDataContainer/VertexData/NodeType"),
    num_sampl_pts=3000,
    phase_of_interest=1,
    save_relative_err=False,
    surface_mesh_face_areas_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    surface_mesh_feature_face_labels_array_path=nx.DataPath("TriangleDataContainer/SharedFeatureFace/FaceLabels"),
    triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.GetDataDirectory() + "/Output/SurfaceMesh/SmallIN100_GBCD_Metric.dream3d"
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