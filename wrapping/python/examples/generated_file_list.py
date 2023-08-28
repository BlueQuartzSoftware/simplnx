# Import the DREAM3D Base library and Plugins
import complex as cx

import itkimageprocessing as cxitk
import orientationanalysis as cxor

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()


generated_file_list_value = cx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = "/Users/mjackson/DREAM3DNXData/Data/Porosity_Image"
generated_file_list_value.ordering = cx.GeneratedFileListParameter.Ordering.LowToHigh

generated_file_list_value.file_prefix = "slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".tif"
generated_file_list_value.start_index = 11
generated_file_list_value.end_index = 174
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 2

result = cxitk.ITKImportImageStack.execute(data_structure=data_structure, 
                                  cell_data_name="Cell Data", 
                                  image_data_array_path="Image Data", 
                                  image_geometry_path=cx.DataPath(["Image Stack"]), 
                                  image_transform_choice=0,
                                   input_file_list_info=generated_file_list_value,
                                   origin=[0., 0., 0.], 
                                   spacing=[1., 1.,1.])
if len(result.errors) != 0:
    print('Errors: {}', result.errors)
    print('Warnings: {}', result.warnings)
else:
    print("No errors running the filter")