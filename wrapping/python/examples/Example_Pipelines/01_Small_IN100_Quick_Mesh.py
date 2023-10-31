import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#Filter 2

result = cx.CropImageGeometry.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    #created_image_geometry=cx.DataPath("DataContainer/"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    max_voxel=[140, 140, 99],
    min_voxel=[41, 41, 0],
    remove_original_geometry=True,
    renumber_features=True,
    selected_image_geometry=cx.DataPath("DataContainer"),
    update_origin=True
)

#Filter 3

result = cx.MoveData.execute(
    data_structure=data_structure,
    data=[cx.DataPath("DataContainer/CellEnsembleData")],
    new_parent=cx.DataPath("DataContainer")
)

#Filter 4

result = cx.QuickSurfaceMeshFilter.execute(
    data_structure=data_structure,
    face_data_group_name=("FaceData"),
    face_feature_attribute_matrix_name=("Face Feature Data"),
    face_labels_array_name=("FaceLabels"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    fix_problem_voxels=False,
    generate_triple_lines=False,
    grid_geometry_data_path=cx.DataPath("DataContainer"),
    node_types_array_name=("NodeType"),
    #selected_data_array_paths=[cx.DataPath("DataContainer")]
    triangle_geometry_name=cx.DataPath("TriangleDataContainer"),
    vertex_data_group_name=("VertexData")
)

#Filter 5

output_file_path = "Data/Output/SurfaceMesh/SmallIN100_Mesh.dream3d"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")