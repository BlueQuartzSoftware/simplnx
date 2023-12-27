import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/Statistics/SmallIN100_CrystalStats.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = cx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                         import_file_data=import_data)

if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter
filter = cx.CropImageGeometry()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_feature_attribute_matrix=cx.DataPath("DataContainer/CellFeatureData"),
    feature_ids=cx.DataPath("DataContainer/CellData/FeatureIds"),
    max_voxel=[140, 140, 99],
    min_voxel=[41, 41, 0],
    remove_original_geometry=True,
    renumber_features=True,
    selected_image_geometry=cx.DataPath("DataContainer"),
    update_origin=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 3
# Instantiate Filter
#filter = cx.MoveData()
# Execute Filter with Parameters
#result = filter.execute(
#    data_structure=data_structure,
#     data=[cx.DataPath("DataContainer/CellEnsembleData")],
#     new_parent=cx.DataPath("DataContainer")
# )
# if len(result.warnings) != 0:
#     print(f'{filter.name()} Warnings: {result.warnings}')
# if len(result.errors) != 0:
#     print(f'{filter.name()} Errors: {result.errors}')
#     quit()
# else:
#     print(f"{filter.name()} No errors running the filter")

# Filter 4
# Instantiate Filter
filter = cx.QuickSurfaceMeshFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_data_group_name=("FaceData"),
    face_feature_attribute_matrix_name=("Face Feature Data"),
    face_labels_array_name=("FaceLabels"),
    feature_ids_path=cx.DataPath("DataContainer/CellData/FeatureIds"),
    fix_problem_voxels=False,
    generate_triple_lines=False,
    grid_geometry_data_path=cx.DataPath("DataContainer"),
    node_types_array_name=("NodeType"),
    triangle_geometry_name=cx.DataPath("TriangleDataContainer"),
    vertex_data_group_name=("VertexData")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 5
# Set Output File Path
output_file_path = "Data/Output/SurfaceMesh/SmallIN100_Mesh.dream3d"
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
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
    print(f"{filter.name()} No errors running the filter")


print("===> Pipeline Complete")