import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import complex_test_dirs as cxtest

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Import Data Parameter
import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "Data/Output/SurfaceMesh/SmallIN100_MeshStats.dream3d"
import_data.data_paths = None
# Instantiate Filter
filter = cx.ReadDREAM3DFilter()
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure, import_file_data=import_data)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ReadDREAM3DFilter filter")

# Filter 2
# Instantiate Filter
filter = cxor.FindGBCDFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    face_ensemble_attribute_matrix_name="FaceEnsembleData",
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    feature_phases_array_path=cx.DataPath("DataContainer/CellFeatureData/Phases"),
    gbcd_array_name="GBCD",
    gbcd_resolution=9.0,
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals"),
    triangle_geometry=cx.DataPath("TriangleDataContainer")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the FindGBCDFilter")


# Filter 3
# Instantiate Filter
filter = cxor.GenerateGBCDPoleFigureFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_intensity_array_name="MRD",
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    image_geometry_name=cx.DataPath("GBCD Pole Figure [Sigma 3]"),
    misorientation_rotation=[60.0, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the GenerateGBCDPoleFigureFilter")

# Filter 4
# Instantiate Filter
filter = cxor.GenerateGBCDPoleFigureFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_intensity_array_name="MRD",
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    image_geometry_name=cx.DataPath("GBCD Pole Figure [Sigma 9]"),
    misorientation_rotation=[39.0, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1
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
filter = cxor.GenerateGBCDPoleFigureFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_intensity_array_name="MRD",
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    image_geometry_name=cx.DataPath("GBCD Pole Figure [Sigma 11]"),
    misorientation_rotation=[50.5, 1.0, 1.0, 1.0],
    output_image_dimension=100,
    phase_of_interest=1
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
filter = cxor.WriteGBCDGMTFileFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    crystal_structures_array_path=cx.DataPath("DataContainer/CellEnsembleData/CrystalStructures"),
    gbcd_array_path=cx.DataPath("TriangleDataContainer/FaceEnsembleData/GBCD"),
    misorientation_rotation=[60.0, 1.0, 1.0, 1.0],
    output_file="Data/Output/SmallIN100GBCD/SmallIn100GMT_1.dat",
    phase_of_interest=1
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the WriteGBCDGMTFileFilter")

# Filter 7
# Instantiate Filter
filter = cxor.WriteGBCDTriangleDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    feature_euler_angles_array_path=cx.DataPath("DataContainer/CellFeatureData/AvgEulerAngles"),
    output_file="Data/Output/SmallIN100GBCD/SmallIn100Triangles.ph",
    surface_mesh_face_areas_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceAreas"),
    surface_mesh_face_labels_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceLabels"),
    surface_mesh_face_normals_array_path=cx.DataPath("TriangleDataContainer/FaceData/FaceNormals")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the WriteGBCDTriangleDataFilter")

# Filter 8
# Instantiate Filter
filter = cx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = "Data/Output/SurfaceMesh/SmallIN100_GBCD.dream3d"
# Execute Filter with Parameters
result = filter.execute(data_structure=data_structure,
                        export_file_path=output_file_path,
                        write_xdmf_file=True
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the WriteDREAM3DFilter")


print("===> Pipeline Complete")