import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

result = nx.ReadStlFileFilter.execute(data_structure=data_structure,
                                            face_attribute_matrix="Face Data" ,
                                            face_normals_data_path="Face Normals",
                                            scale_factor=1,
                                            scale_output=False ,
                                            stl_file_path="Data/STL_Models/Cylinder.stl" ,
                                            triangle_geometry_name=nx.DataPath(["[Triangle Geomtry]"]) ,
                                            vertex_attribute_matrix="Vertex Data")

if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running ImportSTLFile filter")



output_file_path = nxtest.get_data_directory() / "Output/ImportSTLFile.dream3d"
result = nx.WriteDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")
