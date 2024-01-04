import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

#Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter 
filter = nx.CombineStlFilesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix_name="Face Data",
    face_normals_array_name="Face Normals",
    stl_files_path=nxtest.GetDataDirectory() + "/STL_Models",
    triangle_data_container_name=nx.DataPath("TriangleGeometry"),
    vertex_attribute_matrix_name="Vertex Data"
)
nxtest.check_filter_result(filter, result)

# Filter 2
# Instantiate Filter 
filter = nx.WriteDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = nxtest.GetDataDirectory() + "/Output/CombinedStlFiles.dream3d"
result = filter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
nxtest.check_filter_result(filter, result)

# *****************************************************************************
# THIS SECTION IS ONLY HERE FOR CLEANING UP THE CI Machines
# If you are using this code, you should COMMENT out the next line
nxtest.cleanup_test_file(output_file_path)
# *****************************************************************************


print("===> Pipeline Complete")