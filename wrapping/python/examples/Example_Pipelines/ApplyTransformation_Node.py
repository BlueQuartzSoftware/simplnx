import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
result = cx.StlFileReaderFilter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
    triangle_geometry_name=cx.DataPath("Blade"),
    vertex_attribute_matrix="Vertex Data",
)
#Filter 2
result = cx.StlFileReaderFilter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
    triangle_geometry_name=cx.DataPath("Blade (Rotation)"),
    vertex_attribute_matrix="Vertex Data",
)
#Filter 3
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    #cell_attribute_matrix_path=cx.DataPath("[Drop attribute matrix here]"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=0,
    #manual_transformation_matrix: List[List[float]] = ...,
    rotation=[1.0, 0.0, 0.0, 180],
    #scale: List[float] = ...,
    selected_image_geometry=cx.DataPath("Blade Rotation"),
    transformation_type=3,
    translate_geometry_to_global_origin=False,
    #translation: List[float] = ...
)
#Filter 4
result = cx.StlFileReaderFilter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
    triangle_geometry_name=cx.DataPath("Blade [Translation]"),
    vertex_attribute_matrix="Vertex Data",
)
#Filter 5
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    #cell_attribute_matrix_path=cx.DataPath("[Drop attribute matrix here]"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=0,
    #manual_transformation_matrix: List[List[float]] = ...,
    #rotation: List[float] = ...,
    #scale: List[float] = ...,
    selected_image_geometry=cx.DataPath("Blade [Translation]"),
    transformation_type=4,
    translate_geometry_to_global_origin=False,
    translation=[10.0, 10.0, 10.0],
)
#Filter 6
result = cx.StlFileReaderFilter.execute(
    data_structure=data_structure,
    face_attribute_matrix="Face Data",
    face_normals_data_path="Face Normals",
    #scale_factor: float = ...,
    scale_output=False,
    stl_file_path=cx.DataPath("Data/STL_Models/ASTMD638_specimen.stl"),
    triangle_geometry_name=cx.DataPath("Blade [Scale]"),
    vertex_attribute_matrix="Vertex Data",
)
#Filter 7
result = cx.ApplyTransformationToGeometryFilter.execute(
    data_structure=data_structure,
    #cell_attribute_matrix_path=cx.DataPath("[Drop attribute matrix here]"),
    #computed_transformation_matrix: DataPath = ...,
    interpolation_type=0,
    #manual_transformation_matrix: List[List[float]] = ...,
    #rotation: List[float] = ...,
    scale=[0.5, 0.5, 1.0],
    selected_image_geometry=cx.DataPath("Blade [Scale]"),
    transformation_type=5,
    translate_geometry_to_global_origin=False,
    #translation= List[float] = ...,
)
#Filter 8
result =cx.ExportDREAM3DFilter.execute(
    data_structure=data_structure,
    export_file_path=cx.DataPath("Data/Output/ApplyTransformation_Node.dream3d"),
    write_xdmf_file=True,
)