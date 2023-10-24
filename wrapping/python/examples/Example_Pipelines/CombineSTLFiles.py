import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#Filter 1
result = cx.CombineStlFilesFilter.execute(
    data_structure=data_structure,
    face_attribute_matrix_name="Face Data",
    face_normals_array_name="Face Normals",
    stl_files_path=cx.DataPath("Data/STL_Models"),
    triangle_data_container_name=cx.DataPath("TriangleGeometry"),
    vertex_attribute_matrix_name="Vertex Data",
)
#Filter 2
result = cx.ExportDREAM3DFilter.execute(
    data_structure=data_structure,
    export_file_path=cx.DataPath("Data/Output/CombinedStlFiles.dream3d"),
    write_xdmf_file=True,
)