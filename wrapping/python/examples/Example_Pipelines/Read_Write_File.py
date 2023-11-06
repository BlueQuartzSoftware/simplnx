import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

#--------------------------------------------------------------------------------------
# Read / "Import"

import_data = cx.Dream3dImportParameter.ImportData()
import_data.file_path = "FileGoesHere"
import_data.data_paths = None

result = cx.ImportDREAM3DFilter.execute(data_structure=data_structure,
                                         import_file_data=import_data)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the ImportDREAM3DFilter filter")

#--------------------------------------------------------------------------------------
# Write / "Export"

output_file_path = "FileGoesHere"
result = cx.ExportDREAM3DFilter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")

#ArrayThresholdSet.ArrayThreshold

threshold_1 = cx.ArrayThreshold()
threshold_1.array_path = cx.DataPath(["Small IN100", "Scan Data", "Confidence Index"])
threshold_1.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_1.value = 0.1

threshold_2 = cx.ArrayThreshold()
threshold_2.array_path = cx.DataPath(["Small IN100", "Scan Data", "Image Quality"])
threshold_2.comparison = cx.ArrayThreshold.ComparisonType.GreaterThan
threshold_2.value = 120

threshold_set = cx.ArrayThresholdSet()
threshold_set.thresholds = [threshold_1, threshold_2]
result = cx.MultiThresholdObjects.execute(data_structure=data_structure,
                                    array_thresholds=threshold_set,
                                    created_data_path="Mask",
                                    created_mask_type=cx.DataType.boolean)
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the MultiThresholdObjects")




#Copy Paste New Exit Command to All Filters

# Filter 1
# Instantiate Filter 
filter = cx.CombineStlFilesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix_name="Face Data",
    face_normals_array_name="Face Normals",
    stl_files_path="Data/STL_Models",
    triangle_data_container_name=cx.DataPath("TriangleGeometry"),
    vertex_attribute_matrix_name="Vertex Data"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

#Change PathLike to Strings "blahblah",
#Copy Paste New Exit Command
#Change Result = Filter.Execute




#Filter Example

import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

#Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter 
filter = cx.CombineStlFilesFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    face_attribute_matrix_name="Face Data",
    face_normals_array_name="Face Normals",
    stl_files_path="Data/STL_Models",
    triangle_data_container_name=cx.DataPath("TriangleGeometry"),
    vertex_attribute_matrix_name="Vertex Data"
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")

# Filter 2
# Instantiate Filter 
filter = cx.ExportDREAM3DFilter()
# Execute Filter with Parameters
output_file_path = "Data/Output/CombinedStlFiles.dream3d"
result = filter.execute(data_structure=data_structure, 
                                        export_file_path=output_file_path, 
                                        write_xdmf_file=True)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")