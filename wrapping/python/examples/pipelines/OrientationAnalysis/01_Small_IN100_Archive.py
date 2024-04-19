import simplnx as nx

import itkimageprocessing as cxitk
import orientationanalysis as cxor
import simplnx_test_dirs as nxtest

import numpy as np

# Create a Data Structure
data_structure = nx.DataStructure()

# Filter 1
# Instantiate Filter

generated_file_list_value = nx.GeneratedFileListParameter.ValueType()
generated_file_list_value.input_path = str(nxtest.get_data_directory() / "Small_IN100")
generated_file_list_value.ordering = nx.GeneratedFileListParameter.Ordering.HighToLow

generated_file_list_value.file_prefix = "Slice_"
generated_file_list_value.file_suffix = ""
generated_file_list_value.file_extension = ".ang"
generated_file_list_value.start_index = 1
generated_file_list_value.end_index = 117
generated_file_list_value.increment_index = 1
generated_file_list_value.padding_digits = 0

nx_filter = cxor.EbsdToH5EbsdFilter()
# Execute Filter with Parameters
result = nx_filter.execute(
    data_structure=data_structure,
    input_file_list_object =generated_file_list_value,
    output_file_path=nxtest.get_data_directory() / "Output/Reconstruction/Small_IN100.h5ebsd",
    reference_frame_index=0,
    stacking_order_index=1,
    z_spacing=0.25
)

nxtest.check_filter_result(nx_filter, result)

print("===> Pipeline Complete")
