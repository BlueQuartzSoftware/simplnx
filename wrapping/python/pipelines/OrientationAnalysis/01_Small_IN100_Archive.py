import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = cx.DataStructure()

# Filter 1
# Instantiate Filter

generated_file_list_value = cx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = nxtest.GetDataDirectory() + "Data/Small_IN100"
generated_file_list_value.ordering = cx.GeneratedFileListParameter.Ordering.HighToLow

generated_file_list_value.file_prefix = "Slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".ang"
generated_file_list_value.start_index = 1
generated_file_list_value.end_index = 117
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 0

filter = cxor.EbsdToH5EbsdFilter()
# Execute Filter with Parameters
result = filter.execute(
    data_structure=data_structure,
    input_file_list_info=generated_file_list_value,
    output_file_path="Data/Output/Reconstruction/Small_IN100.h5ebsd",
    reference_frame=0,
    stacking_order=1,
    z_spacing=0.25
)
if len(result.warnings) !=0:
    print(f'{filter.name()} Warnings: {result.warnings}')
if len(result.errors) != 0:
    print(f'{filter.name()} Errors: {result.errors}')
    quit()
else:
    print(f"{filter.name()} No errors running the filter")


print("===> Pipeline Complete")