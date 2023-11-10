import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Small IN100"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ReadAngDataFilter for 'Small IN100'")

# Filter 2
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform (Rotate)"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ReadAngDataFilter for 'Transform (Rotate)'")

# Filter 3
# Instantiate Filter
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Rotate]/Cell Data"),
    interpolation_type=1,
    rotation=[0.0, 1.0, 0, 180],
    selected_image_geometry=cx.DataPath("Transform (Rotate)"),
    transformation_type=3,
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # scale: List[float] = ...,  # Not used here
    # translation: List[float] = ...  # Not used here
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ApplyTransformationToGeometryFilter")


# Filter 4
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform (Scale)"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ReadAngDataFilter for 'Transform (Scale)'")

# Filter 5
# Instantiate Filter
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Scale]/Cell Data"),
    interpolation_type=1,
    scale=[2.0, 2.0, 1.0],  # Scale transformation
    selected_image_geometry=cx.DataPath("Transform (Scale)"),
    transformation_type=5,  # Type for scale transformation
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # rotation: List[float] = ...,  # Not used here
    # translation: List[float] = ...  # Not used here
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ApplyTransformationToGeometryFilter")

# Filter 6
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform (Translate)"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 7
# Instantiate Filter
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Translate]/Cell Data"),
    interpolation_type=1,
    selected_image_geometry=cx.DataPath("Transform [Translate]"),
    transformation_type=4,
    translate_geometry_to_global_origin=False,
    translation=[50.0, 0.0, 0.0]
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # rotation: List[float] = ...,  # Not used here
    # scale: List[float] = ...  # Not used here
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ApplyTransformationToGeometryFilter for translation")

# Filter 8
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform [Rotation-Interpolation]"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 9
# Instantiate Filter
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Rotation-Interpolation]/Cell Data"),
    interpolation_type=0,
    rotation=[0.0, 0.0, 1.0, 45.0],
    selected_image_geometry=cx.DataPath("Transform [Rotation-Interpolation]"),
    transformation_type=3,  # Type for rotation transformation
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # scale: List[float] = ...  # Not used here
    # translation: List[float] = ...  # Not used here
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


# Filter 10
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform [Scale-Interpolation]"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
if len(result.warnings) != 0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the ReadAngDataFilter")

# Filter 11
# Instantiate Filter
filter = cx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Scale-Interpolation]/Cell Data"),
    interpolation_type=0,
    scale=[3.0, 3.0, 1.0],
    selected_image_geometry=cx.DataPath("Transform [Scale-Interpolation]"),
    transformation_type=5,  # Type for scale transformation
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # rotation: List[float] = ...,  # Not used here
    # translation: List[float] = ...  # Not used here
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 12
# Instantiate Filter
filter = cx.ExportDREAM3DFilter()
# Set Output File Path
output_file_path = "Data/Output/Transformation/ApplyTransformation_Image.dream3d"
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")