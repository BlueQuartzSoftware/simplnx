import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# ------------------------------------------------------------------------------
# Create a DataArray that is as long as my CSV file (99 Rows in this case)
# ------------------------------------------------------------------------------
# Create a Data Structure
data_structure = cx.DataStructure()


result = cx.CombineStlFilesFilter.execute(data_structure=data_structure ,
                                           face_attribute_matrix_name="Face Data" ,
                                            face_normals_array_name="FaceNormals" ,
                                            stl_files_path="/Users/alexjackson/DREAM3DNXData/Data/STL_Models" ,
                                            triangle_data_container_name=cx.DataPath(["TriangleGeometry"]),
                                            vertex_attribute_matrix_name= "Vertex Data")

if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running CombinesFilesFilter filter")




output_file_path = nxtest.GetDataDirectory() + "/Output/CombinedStlFiles.dream3d"
result = cx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")


    