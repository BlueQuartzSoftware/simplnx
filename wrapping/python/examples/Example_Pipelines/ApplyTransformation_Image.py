import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

data_structure = cx.DataStructure()

#Filter 1
result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Small IN100"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
#Filter 2
result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform (Rotate)"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
#Filter 3
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Rotate]/Cell Data"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=1,
    #manual_transformation_matrix: List[List[float]] = ...,
    rotation=[0.0, 1.0, 0, 180],
    #scale: List[float] = ...,
    selected_image_geometry=cx.DataPath("Transform (Rotate)"),
    transformation_type=3,
    translate_geometry_to_global_origin=False,
    #translation: List[float] = ...
)
#Filter 4
result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform (Scale"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
#Filter 5
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Scale]/Cell Data"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=1,
    #manual_transformation_matrix: List[List[float]] = ...,
    #rotation: List[List[float]] = ...,
    scale=[2.0, 2.0, 1.0],
    selected_image_geometry=cx.DataPath("Transform (Scale)"),
    transformation_type=5,
    translate_geometry_to_global_origin=False,
    #translation: List[float] = ...
)
#Filter 6
result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform (Translate)"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
#Filter 7
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Translate]/Cell Data"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=1,
    #manual_transformation_matrix: List[List[float]] = ...,
    #rotation: List[List[float]] = ...,
    #scale: List[List[float]] = ...,
    selected_image_geometry=cx.DataPath("Transform [Translate]"),
    transformation_type=4,
    translate_geometry_to_global_origin=False,
    translation=[50.0, 0.0, 0.0],
)
#Filter 8
result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform [Rotation-Interpolation]"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
#Filter 9
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Rotation-Interpolation]/Cell Data"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=0,
    #manual_transformation_matrix: List[List[float]] = ...,
    rotation=[0.0, 0.0, 1.0, 45.0],
    #scale: List[List[float]] = ...,
    selected_image_geometry=cx.DataPath("Transform [Rotation-Interpolation]"),
    transformation_type=3,
    translate_geometry_to_global_origin=False,
    #translation: List[float] = ...
)
#Filter 10
result = cxor.ReadAngDataFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_name="Cell Data",
    cell_ensemble_attribute_matrix_name="CellEnsembleData",
    data_container_name=cx.DataPath("Transform [Scale-Interpolation]"),
    input_file=cx.DataPath("Data/Small_IN100/Slice_1.ang")
)
#Filter 11
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    cell_attribute_matrix_path=cx.DataPath("Transform [Scale-Interpolation]/Cell Data"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=0,
    #manual_transformation_matrix: List[List[float]] = ...,
    #rotation: List[List[float]] = ...,
    scale=[3.0, 3.0, 1.0],
    selected_image_geometry=cx.DataPath("Transform [Scale-Interpolation]"),
    transformation_type=5,
    translate_geometry_to_global_origin=False,
    #translation: List[float] = ...
)
#Filter 12
result =cx.ExportDREAM3DFilter.execute(
    data_structure=data_structure,
    export_file_path=cx.DataPath("Data/Output/Transformation/ApplyTransformation_Image.dream3d"),
    write_xdmf_file=True,
)