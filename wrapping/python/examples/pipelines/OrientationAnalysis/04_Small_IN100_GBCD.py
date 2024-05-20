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
import_data.data_paths = None
# Instantiate Filter
nx_filter = nx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure, import_data_object=import_data)
nxtest.check_filter_result(nx_filter, result)

# Filter 2
# Instantiate Filter
nx_filter = cxor.ComputeGBCDFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    face_ensemble_attribute_matrix_name="FaceEnsembleData",
    feature_euler_angles_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=nx.DataPath("DataContainer/CellFeatureData/Phases"),
    gbcd_array_name="GBCD",
    gbcd_resolution=9.0,
    surface_mesh_face_areas_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    input_triangle_geometry_path=nx.DataPath("TriangleDataContainer")
)
nxtest.check_filter_result(nx_filter, result)


# Filter 3
# Instantiate Filter
nx_filter = cxor.ComputeGBCDPoleFigureFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_intensity_array_name="MRD",
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=nx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    output_image_geometry_path=nx.DataPath("GBCD Pole Figure [Sigma 3]"),
    misorientation_rotation=[60.0, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1
)
nxtest.check_filter_result(nx_filter, result)

# Filter 4
# Instantiate Filter
nx_filter = cxor.ComputeGBCDPoleFigureFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_intensity_array_name="MRD",
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=nx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    output_image_geometry_path=nx.DataPath("GBCD Pole Figure [Sigma 9]"),
    misorientation_rotation=[39.0, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1
)
nxtest.check_filter_result(nx_filter, result)

# Filter 5
# Instantiate Filter
nx_filter = cxor.ComputeGBCDPoleFigureFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_intensity_array_name="MRD",
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=nx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    output_image_geometry_path=nx.DataPath("GBCD Pole Figure [Sigma 11]"),
    misorientation_rotation=[50.5, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1
)
nxtest.check_filter_result(nx_filter, result)

# Filter 6
# Instantiate Filter
nx_filter = cxor.WriteGBCDGMTFileFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=nx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=nx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    misorientation_rotation=[60.0, 1.0, 1.0, 1.0],
    output_file="Data/Output/SmallIN100GBCD/SmallIn100GMT_1.dat",
    phase_of_interest=1
)
nxtest.check_filter_result(nx_filter, result)

# Filter 7
# Instantiate Filter
nx_filter = cxor.WriteGBCDTriangleDataFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    feature_euler_angles_array_path=nx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    output_file="Data/Output/SmallIN100GBCD/SmallIn100Triangles.ph",
    surface_mesh_face_areas_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=nx.DataPath("TriangleDataContainer/FaceData/FaceNormals")
)
nxtest.check_filter_result(nx_filter, result)

# Filter 8
# Instantiate Filter
nx_filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.get_data_directory() / "Output/SurfaceMesh/SmallIN100_GBCD.dream3d"
# Execute Filter with Parameters
result = nx_filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
nxtest.check_filter_result(nx_filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************

print("===> Pipeline Complete")
