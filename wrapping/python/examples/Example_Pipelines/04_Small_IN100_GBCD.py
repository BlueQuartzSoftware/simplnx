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

result = cxor.FindGBCDFilter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    face_ensemble_attribute_matrix_name=("FaceEnsembleData"),
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    gbcd_array_name=("GBCD"),
    gbcd_resolution=9.0,
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    triangle_geometry=cx.DataPath("TriangleDataContainer"),
)
#Filter 3

result = cxor.GenerateGBCDPoleFigureFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_intensity_array_name=("MRD"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    image_geometry_name=cx.DataPath("GBCD Pole Figure [Sigma 3]"),
    misorientation_rotation=[60.0, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1,
)
#Filter 4

result = cxor.GenerateGBCDPoleFigureFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_intensity_array_name=("MRD"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    image_geometry_name=cx.DataPath("GBCD Pole Figure [Sigma 9]"),
    misorientation_rotation=[39.0, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1,
)
#Filter 5

result = cxor.GenerateGBCDPoleFigureFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name=("Cell Data"),
    cell_intensity_array_name=("MRD"),
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    image_geometry_name=cx.DataPath("GBCD Pole Figure [Sigma 11]"),
    misorientation_rotation=[50.5, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1,
)
#Filter 6

result = cxor.ExportGBCDGMTFileFilter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/GBCD"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    misorientation_rotation=[60.0, 1.0, 1.0, 1.0],
    output_file=cx.DataPath("Data/Output/SmallIN100GBCD/SmallIn100GMT_1.dat"),
    phase_of_interest=1,
)
#Filter 7

result = cxor.ExportGBCDTriangleDataFilter.execute(
    data_structure=data_structure,
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    output_file=cx.DataPath("Data/Output/SmallIN100GBCD/SmallIn100Triangles.ph"),
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
)
#Filter 8

output_file_path = "Data/Output/SurfaceMesh/SmallIN100_GBCD.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")