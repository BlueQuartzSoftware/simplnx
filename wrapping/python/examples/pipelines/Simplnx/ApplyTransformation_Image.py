import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=nx.DataPath("Small IN100"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=nx.DataPath("Transform [Rotate]"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(filter, result)

# Filter 3
# Instantiate Filter
filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Transform [Rotate]/Cell Data"),
    interpolation_type=1,
    rotation=[0.0, 1.0, 0.0, 180.0],
    selected_image_geometry=nx.DataPath("Transform [Rotate]"),
    transformation_type=3,
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # scale: List[float] = ...,  # Not used here
    # translation: List[float] = ...  # Not used here
)
nxtest.check_filter_result(filter, result)


# Filter 4
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=nx.DataPath("Transform [Scale]"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(filter, result)

# Filter 5
# Instantiate Filter
filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Transform [Scale]/Cell Data"),
    interpolation_type=1,
    scale=[2.0, 2.0, 1.0],  # Scale transformation
    selected_image_geometry=nx.DataPath("Transform [Scale]"),
    transformation_type=5,  # Type for scale transformation
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,
    # manual_transformation_matrix: List[List[float]] = ...,
    # rotation: List[float] = ...,
    # translation: List[float] = ...
)
nxtest.check_filter_result(filter, result)

# Filter 6
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=nx.DataPath("Transform [Translate]"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(filter, result)

# Filter 7
# Instantiate Filter
filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Transform [Translate]/Cell Data"),
    interpolation_type=1,
    selected_image_geometry=nx.DataPath("Transform [Translate]"),
    transformation_type=4,
    translate_geometry_to_global_origin=False,
    translation=[50.0, 0.0, 0.0]
    # computed_transformation_matrix: DataPath = ...,  # Not used here
    # manual_transformation_matrix: List[List[float]] = ...,  # Not used here
    # rotation: List[float] = ...,  # Not used here
    # scale: List[float] = ...  # Not used here
)
nxtest.check_filter_result(filter, result)

# Filter 8
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=nx.DataPath("Transform [Rotation-Interpolation]"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(filter, result)

# Filter 9
# Instantiate Filter
filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Transform [Rotation-Interpolation]/Cell Data"),
    interpolation_type=0,
    rotation=[0.0, 0.0, 1.0, 45.0],
    selected_image_geometry=nx.DataPath("Transform [Rotation-Interpolation]"),
    transformation_type=3,
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,
    # manual_transformation_matrix: List[List[float]] = ...,
    # scale: List[float] = ...,
    # translation: List[float] = ...
)
nxtest.check_filter_result(filter, result)


# Filter 10
# Instantiate Filter
filter = cxor.ReadAngDataFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=nx.DataPath("Transform [Scale-Interpolation]"),
    input_file=nxtest.GetDataDirectory() + "/Small_IN100/Slice_1.ang"
)
nxtest.check_filter_result(filter, result)

# Filter 11
# Instantiate Filter
filter = nx.ApplyTransformationToGeometryFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=nx.DataPath("Transform [Scale-Interpolation]/Cell Data"),
    interpolation_type=0,
    scale=[3.0, 3.0, 1.0],
    selected_image_geometry=nx.DataPath("Transform [Scale-Interpolation]"),
    transformation_type=5,
    translate_geometry_to_global_origin=False
    # computed_transformation_matrix: DataPath = ...,
    # manual_transformation_matrix: List[List[float]] = ...,
    # rotation: List[float] = ...,
    # translation: List[float] = ...
)
nxtest.check_filter_result(filter, result)

# Filter 12
# Instantiate Filter
filter = nx.WriteDREAM3DFilter()
# Set Output File Path
output_file_path = nxtest.GetDataDirectory() + "/Output/Transformation/ApplyTransformation_Image.dream3d"
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    export_file_path=output_file_path,
    write_xdmf_file=True
)
nxtest.check_filter_result(filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************


print("===> Pipeline Complete")